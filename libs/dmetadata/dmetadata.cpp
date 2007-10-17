/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2006-02-23
 * Description : image metadata interface
 *
 * Copyright (C) 2006-2007 by Gilles Caulier <caulier dot gilles at gmail dot com>
 * Copyright (C) 2006-2007 by Marcel Wiesweg <marcel dot wiesweg at gmx dot de>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * ============================================================ */

// C++ includes

#include <cmath>

// Qt includes.

#include <QDomDocument>
#include <QFile>

// KDE includes

#include <klocale.h>
#include <kglobal.h>

// LibKDcraw includes.

#include <libkdcraw/dcrawinfocontainer.h>
#include <libkdcraw/kdcraw.h>

// Local includes.

#include "constants.h"
#include "version.h"
#include "ddebug.h"
#include "dmetadata.h"

namespace Digikam
{

DMetadata::DMetadata()
         : KExiv2Iface::KExiv2()
{
}

DMetadata::DMetadata(const QString& filePath)
         : KExiv2Iface::KExiv2()
{
    load(filePath);
}

DMetadata::~DMetadata()
{
}

bool DMetadata::load(const QString& filePath) const
{
    // In first, we trying to get metadata using Exiv2,
    // else we will use dcraw to extract minimal information.

    if (!KExiv2::load(filePath))
    {
        if (!loadUsingDcraw(filePath))
            return false;
    }

    return true;
}

bool DMetadata::loadUsingDcraw(const QString& filePath) const
{
    KDcrawIface::DcrawInfoContainer identify;
    if (KDcrawIface::KDcraw::rawFileIdentify(identify, filePath))
    {
        long int num=1, den=1;

        if (!identify.model.isNull())
            setExifTagString("Exif.Image.Model", identify.model.toLatin1(), false);

        if (!identify.make.isNull())
            setExifTagString("Exif.Image.Make", identify.make.toLatin1(), false);

        if (!identify.owner.isNull())
            setExifTagString("Exif.Image.Artist", identify.owner.toLatin1(), false);

        if (identify.sensitivity != -1)
            setExifTagLong("Exif.Photo.ISOSpeedRatings", identify.sensitivity, false);

        if (identify.dateTime.isValid())
            setImageDateTime(identify.dateTime, false, false);

        if (identify.exposureTime != -1.0)
        {
            convertToRational(1/identify.exposureTime, &num, &den, 8);
            setExifTagRational("Exif.Photo.ExposureTime", num, den, false);
        }

        if (identify.aperture != -1.0)
        {
            convertToRational(identify.aperture, &num, &den, 8);
            setExifTagRational("Exif.Photo.ApertureValue", num, den, false);
        }

        if (identify.focalLength != -1.0)
        {
            convertToRational(identify.focalLength, &num, &den, 8);
            setExifTagRational("Exif.Photo.FocalLength", num, den, false);
        }

        if (identify.imageSize.isValid())
            setImageDimensions(identify.imageSize, false);

        // A RAW image is always uncalibrated. */
        setImageColorWorkSpace(WORKSPACE_UNCALIBRATED, false);

        return true;
    }

    return false;
}

bool DMetadata::setProgramId(bool on) const
{
    if (on)
    {
        QString version(digikam_version);
        QString software("digiKam");
        return setImageProgramId(software, version);
    }
 
    return true;
}

QString DMetadata::getImageComment() const
{
    if (getFilePath().isEmpty())
        return QString();

    // In first we trying to get image comments, outside of Xmp, Exif, and Iptc.
    // For JPEG, string is extracted from JFIF Comments section.
    // For PNG, string is extracted from iTXt chunck.

    QString comment = getCommentsDecoded();
    if (!comment.isEmpty())
        return comment;

    // In second, we trying to get Exif comments

    if (hasExif())
    {
        QString exifComment = getExifComment();     
        if (!exifComment.isEmpty())
            return exifComment;
    }

    // In third, we trying to get Xmp comments. Language Alternative rule is not yet used.

    if (hasXmp())
    {
        QString xmpComment = getXmpTagStringLangAlt("Xmp.dc.description", QString(), false);
        if (!xmpComment.isEmpty())
            return xmpComment;

        xmpComment = getXmpTagStringLangAlt("Xmp.exif.UserComment", QString(), false);
        if (!xmpComment.isEmpty())
            return xmpComment;


        xmpComment = getXmpTagStringLangAlt("Xmp.tiff.ImageDescription", QString(), false);
        if (!xmpComment.isEmpty())
            return xmpComment;
}

    // In four, we trying to get Iptc comments

    if (hasIptc())
    {
        QString iptcComment = getIptcTagString("Iptc.Application2.Caption", false);
        if (!iptcComment.isEmpty() && !iptcComment.trimmed().isEmpty())
            return iptcComment;
    }

    return QString();
}

bool DMetadata::setImageComment(const QString& comment) const
{
    //See B.K.O #139313: An empty string is also a valid value
    /*if (comment.isEmpty())
          return false;*/
    
    DDebug() << getFilePath() << " ==> Comment: " << comment << endl;

    // In first we set image comments, outside of Exif, Xmp, and Iptc.

    if (!setComments(comment.toUtf8()))
        return false;

    // In Second we write comments into Exif.

    if (!setExifComment(comment))
        return false;

    // In Third we write comments into Xmp. Language Alternative rule is not yet used.

    if (!setXmpTagStringLangAlt("Xmp.dc.description", comment, QString(), false))
        return false;

    if (!setXmpTagStringLangAlt("Xmp.exif.UserComment", comment, QString(), false))
        return false;

    if (!setXmpTagStringLangAlt("Xmp.tiff.ImageDescription", comment, QString(), false))
        return false;

    // In Four we write comments into Iptc.
    // Note that Caption IPTC tag is limited to 2000 char and ASCII charset.

    QString commentIptc = comment;
    commentIptc.truncate(2000);

    if (!setIptcTagString("Iptc.Application2.Caption", commentIptc))
        return false;

    return true;
}

int DMetadata::getImageRating() const
{
    if (getFilePath().isEmpty())
        return -1;

    if (hasXmp())
    {
        QString value = getXmpTagString("Xmp.xmp.Rating", false);
        if (!value.isEmpty())
        {
            bool ok     = false;
            long rating = value.toLong(&ok);
            if (ok && rating >= RatingMin && rating <= RatingMax)
                return rating;            
        }
    }

    // Check Exif rating tag set by Windows Vista
    // Note : no need to check rating in percent tags (Exif.image.0x4747) here because 
    // its appear always with rating tag value (Exif.image.0x4749). 

    if (hasExif())
    {
        long rating = -1;
        if (getExifTagLong("Exif.Image.0x4746", rating))
        {
            if (rating >= RatingMin && rating <= RatingMax)
                return rating;            
        }
    }

    // digiKam 0.9.x has used Iptc Urgency to store Rating.
    // This way is obsolete now since digiKam support Xmp.
    // But we will let the capability to import it.
    // Iptc.Application2.Urgency <==> digiKam Rating links:
    //
    // digiKam     IPTC
    // Rating      Urgency
    //    
    // 0 star  <=>  8          // Least important
    // 1 star  <=>  7
    // 1 star  <==  6
    // 2 star  <=>  5
    // 3 star  <=>  4
    // 4 star  <==  3
    // 4 star  <=>  2
    // 5 star  <=>  1          // Most important

    if (hasIptc())
    {
        QString IptcUrgency(getIptcTagData("Iptc.Application2.Urgency"));
        
        if (!IptcUrgency.isEmpty())
        {
            if (IptcUrgency == QString("1"))
                return 5;
            else if (IptcUrgency == QString("2"))
                return 4;
            else if (IptcUrgency == QString("3"))
                return 4;
            else if (IptcUrgency == QString("4"))
                return 3;
            else if (IptcUrgency == QString("5"))
                return 2;
            else if (IptcUrgency == QString("6"))
                return 1;
            else if (IptcUrgency == QString("7"))
                return 1;
            else if (IptcUrgency == QString("8"))
                return 0;
        }
    }

    return -1;
}

bool DMetadata::setImageRating(int rating) const
{
    // NOTE : with digiKam 0.9.x, we have used Iptc Urgency to store Rating. 
    // Now this way is obsolete, and we use standard Xmp rating tag instead.

    if (rating < RatingMin || rating > RatingMax)
    {
        DDebug() << "Rating value to write is out of range!" << endl;
        return false;
    }

    DDebug() << getFilePath() << " ==> Rating: " << rating << endl;

    if (!setProgramId())
        return false;

    // Set standard Xmp rating tag.

    if (!setXmpTagString("Xmp.xmp.Rating", QString::number(rating)))
        return false;

    // Set Exif rating tag used by Windows Vista.

    if (!setExifTagLong("Exif.Image.0x4746", rating))
        return false;

    // Wrapper around rating percents managed by Windows Vista.
    int ratePercents = 0;
    switch(rating)
    {
        case 0:
            ratePercents = 0;
            break;
        case 1:
            ratePercents = 1;
            break;
        case 2:
            ratePercents = 25;
            break;
        case 3:
            ratePercents = 50;
            break;
        case 4:
            ratePercents = 75;
            break;
        case 5:
            ratePercents = 99;
            break;
    }

    if (!setExifTagLong("Exif.Image.0x4749", ratePercents))
        return false;

    return true;
}

PhotoInfoContainer DMetadata::getPhotographInformations() const
{
    PhotoInfoContainer photoInfo;

    if (hasExif() || hasXmp())
    {
        photoInfo.dateTime = getImageDateTime();

        photoInfo.make     = getExifTagString("Exif.Image.Make");
        if (photoInfo.make.isEmpty())
            photoInfo.make = getXmpTagString("Xmp.tiff.Make");

        photoInfo.model    = getExifTagString("Exif.Image.Model");
        if (photoInfo.model.isEmpty())
            photoInfo.model = getXmpTagString("Xmp.tiff.Model");

        photoInfo.aperture = getExifTagString("Exif.Photo.FNumber");
        if (photoInfo.aperture.isEmpty())
        {
            photoInfo.aperture = getExifTagString("Exif.Photo.ApertureValue");
            if (photoInfo.aperture.isEmpty())
            {
                photoInfo.aperture = getXmpTagString("Xmp.exif.FNumber");
                if (photoInfo.aperture.isEmpty())
                    photoInfo.aperture = getXmpTagString("Xmp.exif.ApertureValue");
            }
        }

        photoInfo.exposureTime = getExifTagString("Exif.Photo.ExposureTime");
        if (photoInfo.exposureTime.isEmpty())
        {
            photoInfo.exposureTime = getExifTagString("Exif.Photo.ShutterSpeedValue");
            if (photoInfo.exposureTime.isEmpty())
            {
                photoInfo.exposureTime = getXmpTagString("Xmp.exif.ExposureTime");
                if (photoInfo.exposureTime.isEmpty())
                    photoInfo.exposureTime = getXmpTagString("Xmp.exif.ShutterSpeedValue");
            }
        }

        photoInfo.exposureMode    = getExifTagString("Exif.Photo.ExposureMode");
        if (photoInfo.exposureMode.isEmpty())
            photoInfo.exposureMode = getXmpTagString("Xmp.exif.ExposureMode");

        photoInfo.exposureProgram = getExifTagString("Exif.Photo.ExposureProgram");
        if (photoInfo.exposureProgram.isEmpty())
            photoInfo.exposureProgram = getXmpTagString("Xmp.exif.ExposureProgram");

        photoInfo.focalLength     = getExifTagString("Exif.Photo.FocalLength");
        if (photoInfo.focalLength.isEmpty())
            photoInfo.focalLength = getXmpTagString("Xmp.exif.FocalLength");

        photoInfo.focalLength35mm = getExifTagString("Exif.Photo.FocalLengthIn35mmFilm");
        if (photoInfo.focalLength35mm.isEmpty())
            photoInfo.focalLength35mm = getXmpTagString("Xmp.exif.FocalLengthIn35mmFilm");

        photoInfo.sensitivity = getExifTagString("Exif.Photo.ISOSpeedRatings");
        if (photoInfo.sensitivity.isEmpty())
        {
            photoInfo.sensitivity = getExifTagString("Exif.Photo.ExposureIndex");
            if (photoInfo.sensitivity.isEmpty())
            {
                photoInfo.sensitivity = getXmpTagString("Xmp.exif.ISOSpeedRatings");
                if (photoInfo.sensitivity.isEmpty())
                    photoInfo.sensitivity = getXmpTagString("Xmp.exif.ExposureIndex");
            }
        }

        photoInfo.flash = getExifTagString("Exif.Photo.Flash");
        if (photoInfo.flash.isEmpty())
            photoInfo.flash = getXmpTagString("Xmp.exif.Flash");

        photoInfo.whiteBalance = getExifTagString("Exif.Photo.WhiteBalance");
        if (photoInfo.whiteBalance.isEmpty())
            photoInfo.whiteBalance = getXmpTagString("Xmp.exif.WhiteBalance");
    }

    return photoInfo;
}

bool DMetadata::getImageTagsPath(QStringList& tagsPath) const
{
    // Standard Xmp namespace do not provide a place to store Tags Path list as well. 
    // We will use a private namespace for that.
    if (!registerXmpNameSpace("http://www.digikam.org/ns/1.0/", "digiKam"))
        return false;

    // Try to get Tags Path list from Xmp in first.
    tagsPath = getXmpTagStringSeq("Xmp.digiKam.TagsList", false);
    if (!tagsPath.isEmpty())
        return true;

    // Try to get Tags Path list from Xmp keywords.
    tagsPath = getXmpKeywords();
    if (!tagsPath.isEmpty())
        return true;

    // Try to get Tags Path list from Iptc keywords.
    // digiKam 0.9.x has used Iptc keywords to store Tags Path list.
    // This way is obsolete now since digiKam support Xmp because Iptc 
    // do not support UTF-8 and have strings size limitation. But we will 
    // let the capability to import it for interworking issues.
    tagsPath = getIptcKeywords();
    if (!tagsPath.isEmpty())
        return true;
        
    return false;
}

bool DMetadata::setImageTagsPath(const QStringList& tagsPath) const
{
    // NOTE : with digiKam 0.9.x, we have used Iptc Keywords for that. 
    // Now this way is obsolete, and we use Xmp instead.

    // Standard Xmp namespace do not provide a place to store Tags path as well. 
    // We will use a private namespace for that.
    if (!registerXmpNameSpace("http://www.digikam.org/ns/1.0/", "digiKam"))
        return false;

    // Remove the old Tags path list from metadata if already exist.
    if (removeXmpTag("Xmp.digiKam.TagsList", false))
        return false;

    // An now, add the new Tags path list as well.
    if (setXmpTagStringSeq("Xmp.digiKam.TagsList", tagsPath))
        return false;

    return true;
}

bool DMetadata::setImagePhotographerId(const QString& author, const QString& authorTitle) const
{
    if (!setProgramId())
        return false;

    // Set Xmp tags. Xmp<->Iptc Schema from Photoshop 7.0

    // Create a list of authors including old one witch already exists.
    QStringList oldAuthors = getXmpTagStringSeq("Xmp.dc.creator", false);
    QStringList newAuthors(author);

    for (QStringList::Iterator it = oldAuthors.begin(); it != oldAuthors.end(); ++it )
    {
        if (!newAuthors.contains(*it))
            newAuthors.append(*it);
    }

    if (!setXmpTagStringSeq("Xmp.dc.creator", newAuthors, false))
        return false;

    if (!setXmpTagStringSeq("Xmp.tiff.Artist", newAuthors, false))
        return false;

    if (!setXmpTagString("Xmp.photoshop.AuthorsPosition", authorTitle, false))
        return false;

    // Set Iptc tags. 

    if (!setIptcTag(author,      32, "Author",       "Iptc.Application2.Byline"))      return false;
    if (!setIptcTag(authorTitle, 32, "Author Title", "Iptc.Application2.BylineTitle")) return false;

    return true;
}

bool DMetadata::setImageCredits(const QString& credit, const QString& source, const QString& copyright) const
{
    if (!setProgramId())
        return false;

    // Set Xmp tags. Xmp<->Iptc Schema from Photoshop 7.0

    if (!setXmpTagString("Xmp.photoshop.Credit", credit, false))
        return false;

    if (!setXmpTagString("Xmp.photoshop.Source", source, false))
        return false;

    if (!setXmpTagString("Xmp.dc.source", source, false))
        return false;

    // NOTE : language Alternative rule is not yet used here.
    if (!setXmpTagStringLangAlt("Xmp.dc.rights", copyright, QString(), false))
        return false;

    if (!setXmpTagStringLangAlt("Xmp.tiff.Copyright", copyright, QString(), false))
        return false;

    // Set Iptc tags.

    if (!setIptcTag(credit,     32, "Credit",    "Iptc.Application2.Credit"))    return false;
    if (!setIptcTag(source,     32, "Source",    "Iptc.Application2.Source"))    return false;
    if (!setIptcTag(copyright, 128, "Copyright", "Iptc.Application2.Copyright")) return false;

    return true;
}

bool DMetadata::setIptcTag(const QString& text, int maxLength,
                           const char* debugLabel, const char* tagKey)  const
{
    QString truncatedText = text;
    truncatedText.truncate(maxLength);
    DDebug() << getFilePath() << " ==> " << debugLabel << ": " << truncatedText << endl;
    return setIptcTagString(tagKey, truncatedText);    // returns false if failed
}

inline QVariant DMetadata::fromExifOrXmp(const char *exifTagName, const char *xmpTagName)
{
    QVariant var;

    var = getExifTagVariant(exifTagName, false);
    if (!var.isNull())
        return var;

    if (xmpTagName)
    {
        //TODO
    }

    return var;
}

inline QVariant DMetadata::fromIptcOrXmp(const char *iptcTagName, const char *xmpTagName)
{
    QString iptcValue;

    if (iptcTagName)
        iptcValue = getIptcTagString(iptcTagName);
    if (!iptcValue.isNull())
        return iptcValue;

    if (xmpTagName)
    {
        // TODO
    }

    return QVariant();
}

inline QVariant DMetadata::fromIptcOrXmpList(const char *iptcTagName, const char *xmpTagName)
{
    QStringList iptcValues;

    if (iptcTagName)
        iptcValues = getIptcTagsStringList(iptcTagName);
    if (!iptcValues.isEmpty())
        return iptcValues;

    if (xmpTagName)
    {
        // TODO
    }

    return QVariant();
}

inline QVariant DMetadata::fromIptcOrXmpLangAlt(const char *iptcTagName, const char *xmpTagName)
{
    QString iptcValue;

    if (iptcTagName)
        iptcValue = getIptcTagString(iptcTagName);
    if (!iptcValue.isNull())
    {
        QMap<QString, QVariant> map;
        map["x-default"] = iptcValue;
        return map;
    }

    if (xmpTagName)
    {
        // TODO
    }

    return QVariant();
}

QVariant DMetadata::getMetadataField(MetadataInfo::Field field)
{
    switch (field)
    {
        case MetadataInfo::Rating:
            return getImageRating();
        case MetadataInfo::CreationDate:
            return getImageDateTime();
        case MetadataInfo::DigitizationDate:
            return getDigitizationDateTime(true);
        case MetadataInfo::Orientation:
            return getImageOrientation();

        case MetadataInfo::Make:
            return fromExifOrXmp("Exif.Image.Make", "tiff.Make");
        case MetadataInfo::Model:
            return fromExifOrXmp("Exif.Image.Model", "tiff.Model");
        case MetadataInfo::Aperture:
        {
            QVariant var = fromExifOrXmp("Exif.Photo.FNumber", "exif.FNumber");
            if (var.isNull())
            {
                var = fromExifOrXmp("Exif.Photo.ApertureValue", "exif.ApertureValue");
                if (!var.isNull())
                    var = apexApertureToFNumber(var.toDouble());
            }
            return var;
        }
        case MetadataInfo::FocalLength:
            return fromExifOrXmp("Exif.Photo.FocalLength", "exif.FocalLength");
        case MetadataInfo::FocalLengthIn35mm:
            return fromExifOrXmp("Exif.Photo.FocalLengthIn35mmFilm", "exif.FocalLengthIn35mmFilm");
        case MetadataInfo::ExposureTime:
        {
            QVariant var = fromExifOrXmp("Exif.Photo.ExposureTime", "exif.ExposureTime");
            if (var.isNull())
            {
                var = fromExifOrXmp("Exif.Photo.ShutterSpeedValue", "exif.ShutterSpeedValue");
                if (!var.isNull())
                    var = apexShutterSpeedToExposureTime(var.toDouble());
            }
            return var;
        }
        case MetadataInfo::ExposureProgram:
            return fromExifOrXmp("Exif.Photo.ExposureProgram", "exif.ExposureProgram");
        case MetadataInfo::ExposureMode:
            return fromExifOrXmp("Exif.Photo.ExposureMode", "exif.ExposureMode");
        case MetadataInfo::Sensitivity:
        {
            QVariant var = fromExifOrXmp("Exif.Photo.ISOSpeedRatings", "exif.ISOSpeedRatings");
            //if (var.isNull())
                // TODO: has this ISO format??? We must convert to the format of ISOSpeedRatings!
              //  var = fromExifOrXmp("Exif.Photo.ExposureIndex", "exif.ExposureIndex");
            return var;
        }
        case MetadataInfo::FlashMode:
            return fromExifOrXmp("Exif.Photo.Flash", "exif.Flash");
        case MetadataInfo::WhiteBalance:
            return fromExifOrXmp("Exif.Photo.WhiteBalance", "exif.WhiteBalance");
        case MetadataInfo::MeteringMode:
            return fromExifOrXmp("Exif.Photo.MeteringMode", "exif.MeteringMode");
        case MetadataInfo::SubjectDistance:
            return fromExifOrXmp("Exif.Photo.SubjectDistance", "exif.SubjectDistance");
        case MetadataInfo::SubjectDistanceCategory:
            return fromExifOrXmp("Exif.Photo.SubjectDistanceRange", "exif.SubjectDistanceRange");
        case MetadataInfo::WhiteBalanceColorTemperature:
            //TODO: ??
            return QVariant();

        case MetadataInfo::Longitude:
            return getGPSLongitudeString();
        case MetadataInfo::LongitudeNumber:
        {
            double longitude;
            if (getGPSLongitudeNumber(&longitude))
                return longitude;
            else
                return QVariant();
        }
        case MetadataInfo::Latitude:
            return getGPSLatitudeString();
        case MetadataInfo::LatitudeNumber:
        {
            double latitude;
            if (getGPSLatitudeNumber(&latitude))
                return latitude;
            else
                return QVariant();
        }
        case MetadataInfo::Altitude:
        {
            double altitude;
            if (getGPSAltitude(&altitude))
                return altitude;
            else
                return QVariant();
        }
        case MetadataInfo::GeographicOrientation:
        case MetadataInfo::CameraTilt:
        case MetadataInfo::CameraRoll:
        case MetadataInfo::PositionDescription:
            // TODO or unsupported?
            return QVariant();

        //TODO: Check all IPTC tag names
        case MetadataInfo::IPTCCoreCopyrightNotice:
            return fromIptcOrXmpLangAlt("Iptc.Application2.Copyright", "dc.rights");
        case MetadataInfo::IPTCCoreCreator:
            return fromIptcOrXmpList("Iptc.Application2.Creator", "dc.creator");
        case MetadataInfo::IPTCCoreProvider:
            return fromIptcOrXmp("Iptc.Application2.Credit", "photoshop.Credit");
        case MetadataInfo::IPTCCoreRightUsageTerms:
            return fromIptcOrXmpLangAlt(0, "xmpRights.UsageTerms");
        case MetadataInfo::IPTCCoreSource:
            return fromIptcOrXmp("Iptc.Application2.Source", "photoshop.Source");

        case MetadataInfo::IPTCCoreCreatorJobTitle:
            return fromIptcOrXmp("Iptc.Application2.BylineTitle", "photoshop.AuthorsPosition");
        case MetadataInfo::IPTCCoreInstructions:
            return fromIptcOrXmp("Iptc.Application2.SpecialInstructions", "photoshop.Instructions");

        case MetadataInfo::IPTCCoreCountryCode:
            return fromIptcOrXmp("Iptc.Application2.CountryCode", "Iptc4xmpCore.CountryCode");
        case MetadataInfo::IPTCCoreCountry:
            return fromIptcOrXmp("Iptc.Application2.Country", "photoshop.Country");
        case MetadataInfo::IPTCCoreCity:
            return fromIptcOrXmp("Iptc.Application2.City", "photoshop.City");
        case MetadataInfo::IPTCCoreLocation:
            return fromIptcOrXmp("Iptc.Application2.Sublocation", "Iptc4xmpCore.Location");
        case MetadataInfo::IPTCCoreProvinceState:
            return fromIptcOrXmp("Iptc.Application2.State", "photoshop.State");
        case MetadataInfo::IPTCCoreIntellectualGenre:
            return fromIptcOrXmp("Iptc.Application2.ObjectAttributesReference", "Iptc4xmpCore.IntellectualGenre");
        case MetadataInfo::IPTCCoreJobID:
            return fromIptcOrXmp("Iptc.Application2.OriginalTransmissionReference", "photoshop.TransmissionReference");
        case MetadataInfo::IPTCCoreScene:
            return fromIptcOrXmpList(0, "Iptc4xmpCore.Scene");
        case MetadataInfo::IPTCCoreSubjectCode:
            return fromIptcOrXmpList("Iptc.Application2.SubjectReference", "Iptc4xmpCore.SubjectCode");

        case MetadataInfo::IPTCCoreDescription:
            return fromIptcOrXmpLangAlt("Iptc.Application2.Caption", "dc.description");
        case MetadataInfo::IPTCCoreDescriptionWriter:
            return fromIptcOrXmp("Iptc.Application2.Writer", "photoshop.CaptionWriter");
        case MetadataInfo::IPTCCoreHeadline:
            return fromIptcOrXmp("Iptc.Application2.Headline", "photoshop.Headline");
        case MetadataInfo::IPTCCoreTitle:
            return fromIptcOrXmpLangAlt("Iptc.Application2.ObjectName", "photoshop.dc.title");
        default:
            return QVariant();
    }
}

QVariantList DMetadata::getMetadataFields(const MetadataFields &fields)
{
    QVariantList list;
    foreach (MetadataInfo::Field field, fields)
    {
        list << getMetadataField(field);
    }
    return list;
}

QString DMetadata::valueToString (const QVariant &value, MetadataInfo::Field field)
    switch (field)
    {
        case MetadataInfo::Rating:
            return value.toString();
        case MetadataInfo::CreationDate:
        case MetadataInfo::DigitizationDate:
            return value.toDateTime().toString(Qt::LocaleDate);
        case MetadataInfo::Orientation:
            switch (value.toInt())
            {
                // Example why the English text differs from the enum names: ORIENTATION_ROT_90.
                // Rotation by 90� is right (clockwise) rotation.
                // But: The enum names describe what needs to be done to get the image right again.
                // And an image that needs to be rotated 90� is currently rotated 270� = left.

                case ORIENTATION_UNSPECIFIED:
                    return i18n("Unspecified");
                case ORIENTATION_NORMAL:
                    return i18nc("Rotation of an unrotated image", "Normal");
                case ORIENTATION_HFLIP:
                    return i18n("Flipped Horizontally");
                case ORIENTATION_ROT_180:
                    return i18n("Rotated by 180 Degrees");
                case ORIENTATION_VFLIP:
                    return i18n("Flipped Vertically");
                case ORIENTATION_ROT_90_HFLIP:
                    return i18n("Flipped Horizontally and Rotated Left");
                case ORIENTATION_ROT_90:
                    return i18n("Rotated Left");
                case ORIENTATION_ROT_90_VFLIP:
                    return i18n("Flipped Vertically and Rotated Left");
                case ORIENTATION_ROT_270:
                    return i18n("Rotated Right");
            }

        case MetadataInfo::Make:
            return createExifUserStringFromValue("Exif.Image.Make", value);
        case MetadataInfo::Model:
            return createExifUserStringFromValue("Exif.Image.Model", value);
        case MetadataInfo::Aperture:
            return createExifUserStringFromValue("Exif.Photo.FNumber", value);
        case MetadataInfo::FocalLength:
            return createExifUserStringFromValue("Exif.Photo.FocalLength", value);
        case MetadataInfo::FocalLengthIn35mm:
            return createExifUserStringFromValue("Exif.Photo.FocalLengthIn35mmFilm", value);
        case MetadataInfo::ExposureTime:
            return createExifUserStringFromValue("Exif.Photo.ExposureTime", value);
        case MetadataInfo::ExposureProgram:
            return createExifUserStringFromValue("Exif.Photo.ExposureProgram", value);
        case MetadataInfo::ExposureMode:
            return createExifUserStringFromValue("Exif.Photo.ExposureMode", value);
        case MetadataInfo::Sensitivity:
            return createExifUserStringFromValue("Exif.Photo.ISOSpeedRatings", value);
        case MetadataInfo::FlashMode:
            return createExifUserStringFromValue("Exif.Photo.Flash", value);
        case MetadataInfo::WhiteBalance:
            return createExifUserStringFromValue("Exif.Photo.WhiteBalance", value);
        case MetadataInfo::MeteringMode:
            return createExifUserStringFromValue("Exif.Photo.MeteringMode", value);
        case MetadataInfo::SubjectDistance:
            return createExifUserStringFromValue("Exif.Photo.SubjectDistance", value);
        case MetadataInfo::SubjectDistanceCategory:
            return createExifUserStringFromValue("Exif.Photo.SubjectDistanceRange", value);
        case MetadataInfo::WhiteBalanceColorTemperature:
            return i18nc("Temperature in Kelvin", "%1 K", value.toInt());

        case MetadataInfo::Longitude:
        {
            int degrees, minutes;
            double seconds;
            char directionRef;
            if (!convertToUserPresentableNumbers(value.toString(), &degrees, &minutes, &seconds, &directionRef))
                return QString();
            QString direction = (directionRef == 'W') ?
                                i18nc("For use in longitude coordinate", "West") : i18nc("For use in longitude coordinate'' East", "East");
            return QString("%1�%2'%3'' %4").arg(degrees).arg(minutes).arg(seconds).arg(direction);
        }
        case MetadataInfo::LongitudeNumber:
        {
            int degrees, minutes;
            double seconds;
            char directionRef;
            convertToUserPresentableNumbers(false, value.toDouble(), &degrees, &minutes, &seconds, &directionRef);
            QString direction = (directionRef == 'W') ?
                                i18nc("For use in longitude coordinate", "West") : i18nc("For use in longitude coordinate'' East", "East");
            return QString("%1�%2'%3'' %4").arg(degrees).arg(minutes).arg(seconds).arg(direction);
        }
        case MetadataInfo::Latitude:
        {
            int degrees, minutes;
            double seconds;
            char directionRef;
            if (!convertToUserPresentableNumbers(value.toString(), &degrees, &minutes, &seconds, &directionRef))
                return QString();
            QString direction = (directionRef == 'N') ?
                                i18nc("For use in latitude coordinate", "North") : i18nc("For use in latitude coordinate'' South", "South");
            return QString("%1�%2'%3'' %4").arg(degrees).arg(minutes).arg(seconds).arg(direction);
        }
        case MetadataInfo::LatitudeNumber:
        {
            int degrees, minutes;
            double seconds;
            char directionRef;
            convertToUserPresentableNumbers(false, value.toDouble(), &degrees, &minutes, &seconds, &directionRef);
            QString direction = (directionRef == 'N') ?
                                i18nc("For use in latitude coordinate", "North") : i18nc("For use in latitude coordinate'' East", "North");
            return QString("%1�%2'%3'' %4").arg(degrees).arg(minutes).arg(seconds).arg(direction);
        }
        case MetadataInfo::Altitude:
        {
            QString meters = QString("%L1").arg(value.toDouble(), 0, 'f', 2);
            return i18nc("Height in meters", "%1m", meters);
        }

        case MetadataInfo::GeographicOrientation:
        case MetadataInfo::CameraTilt:
        case MetadataInfo::CameraRoll:
            //TODO
            return value.toString();
        case MetadataInfo::PositionDescription:
            return value.toString();

        // Lang Alt
        case MetadataInfo::IPTCCoreCopyrightNotice:
        case MetadataInfo::IPTCCoreRightUsageTerms:
        case MetadataInfo::IPTCCoreDescription:
        case MetadataInfo::IPTCCoreTitle:
        {
            QMap<QString, QVariant> map = value.toMap();
            // the most common cases
            if (map.isEmpty())
                return QString();
            else if (map.size() == 1)
                return map.begin().value().toString();
            // Try "en-us"
            KLocale *locale = KGlobal::locale();
            QString spec = locale->language().toLower() + '-' + locale->country().toLower();
            if (map.contains(spec))
                return map[spec].toString();

            // Try "en-"
            QStringList keys = map.keys();
            QRegExp exp(locale->language().toLower() + '-');
            QStringList matches = keys.filter(exp);
            if (!matches.isEmpty())
                return map[matches.first()].toString();

            // return default
            if (map.contains("x-default"))
                return map["x-default"].toString();

            // return first entry
            return map.begin().value().toString();
        }

        // List
        case MetadataInfo::IPTCCoreCreator:
        case MetadataInfo::IPTCCoreScene:
        case MetadataInfo::IPTCCoreSubjectCode:
            return value.toStringList().join(" ");

        // Text
        case MetadataInfo::IPTCCoreProvider:
        case MetadataInfo::IPTCCoreSource:
        case MetadataInfo::IPTCCoreCreatorJobTitle:
        case MetadataInfo::IPTCCoreInstructions:
        case MetadataInfo::IPTCCoreCountryCode:
        case MetadataInfo::IPTCCoreCountry:
        case MetadataInfo::IPTCCoreCity:
        case MetadataInfo::IPTCCoreLocation:
        case MetadataInfo::IPTCCoreProvinceState:
        case MetadataInfo::IPTCCoreIntellectualGenre:
        case MetadataInfo::IPTCCoreJobID:
        case MetadataInfo::IPTCCoreDescriptionWriter:
        case MetadataInfo::IPTCCoreHeadline:
            return value.toString();

        default:
            return QString();
    }
}

QStringList DMetadata::valuesToString(const QVariantList &values, const MetadataFields &fields)
{
    int size = values.size();
    Q_ASSERT(size == values.size());

    QStringList list;
    for (int i=0; i<size; i++)
    {
        list << valueToString(values[i], fields[i]);
    }
    return list;
}

QMap<int, QString> DMetadata::possibleValuesForEnumField(MetadataInfo::Field field)
{
    QMap<int, QString> map;
    int min, max;
    switch (field)
    {
        case MetadataInfo::Orientation:                      /// Int, enum from libkexiv2
            min = ORIENTATION_UNSPECIFIED;
            max = ORIENTATION_ROT_270;
            break;
        case MetadataInfo::ExposureProgram:                  /// Int, enum from Exif
            min = 0;
            max = 8;
            break;
        case MetadataInfo::ExposureMode:                     /// Int, enum from Exif
            min = 0;
            max = 2;
            break;
        case MetadataInfo::WhiteBalance:                     /// Int, enum from Exif
            min = 0;
            max = 1;
            break;
        case MetadataInfo::MeteringMode:                     /// Int, enum from Exif
            min = 0;
            max = 6;
            map[255] = valueToString(255, field);
            break;
        case MetadataInfo::SubjectDistanceCategory:          /// int, enum from Exif
            min = 0;
            max = 3;
            break;
        case MetadataInfo::FlashMode:                        /// Int, bit mask from Exif
            // This one is a bit special.
            // We return a bit mask for binary AND searching.
            map[0x1] = i18n("Flash has been fired");
            map[0x40] = i18n("Flash with red-eye reduction mode");
            //more: TODO?
            return map;
        default:
            DWarning() << "Unsupported field " << field << " in DMetadata::possibleValuesForEnumField" << endl;
            return map;
    }

    for (int i = min; i < max; i++)
    {
        map[i] = valueToString(i, field);
    }
    return map;
}

double DMetadata::apexApertureToFNumber(double aperture)
{
    // convert from APEX. See Exif spec, Annex C.
    if (aperture == 0.0)
        return 1;
    else if (aperture == 1.0)
        return 1.4;
    else if (aperture == 2.0)
        return 2;
    else if (aperture == 3.0)
        return 2.8;
    else if (aperture == 4.0)
        return 4;
    else if (aperture == 5.0)
        return 5.6;
    else if (aperture == 6.0)
        return 8;
    else if (aperture == 7.0)
        return 11;
    else if (aperture == 8.0)
        return 16;
    else if (aperture == 9.0)
        return 22;
    else if (aperture == 10.0)
        return 32;
    return exp2(aperture / 2.0);
}

double DMetadata::apexShutterSpeedToExposureTime(double shutterSpeed)
{
    // convert from APEX. See Exif spec, Annex C.
    if (shutterSpeed == -5.0)
        return 30;
    else if (shutterSpeed == -4.0)
        return 15;
    else if (shutterSpeed == -3.0)
        return 8;
    else if (shutterSpeed == -2.0)
        return 4;
    else if (shutterSpeed == -1.0)
        return 2;
    else if (shutterSpeed == 0.0)
        return 1;
    else if (shutterSpeed == 1.0)
        return 0.5;
    else if (shutterSpeed == 2.0)
        return 0.25;
    else if (shutterSpeed == 3.0)
        return 0.125;
    else if (shutterSpeed == 4.0)
        return 1.0 / 15.0;
    else if (shutterSpeed == 5.0)
        return 1.0 / 30.0;
    else if (shutterSpeed == 6.0)
        return 1.0 / 60.0;
    else if (shutterSpeed == 7.0)
        return 0.008; // 1/125
    else if (shutterSpeed == 8.0)
        return 0.004; // 1/250
    else if (shutterSpeed == 9.0)
        return 0.002; // 1/500
    else if (shutterSpeed == 10.0)
        return 0.001; // 1/1000
    else if (shutterSpeed == 11.0)
        return 0.0005; // 1/2000
    // additions by me
    else if (shutterSpeed == 12.0)
        return 0.00025; // 1/4000
    else if (shutterSpeed == 13.0)
        return 0.000125; // 1/8000

    return exp2( - shutterSpeed);
}

/**
The following methods set and get an XML dataset into a private IPTC.Application2 tags 
to backup digiKam image properties. The XML text data are compressed using zlib and stored 
like a byte array. The XML text data format are like below:

<?xml version="1.0" encoding="UTF-8"?>
<digikamproperties>
 <comments value="A cool photo from Adrien..." />
 <date value="2006-11-23T13:36:26" />
 <rating value="4" />
 <tagslist>
  <tag path="Gilles/Adrien/testphoto" />
  <tag path="monuments/Trocadero/Tour Eiffel" />
  <tag path="City/Paris" />
 </tagslist>
</digikamproperties>

*/

bool DMetadata::getXMLImageProperties(QString& comments, QDateTime& date, 
                                      int& rating, QStringList& tagsPath) const
{
    rating = 0;

    QByteArray data = getIptcTagData("Iptc.Application2.0x00ff");
    if (data.isEmpty())
        return false;
    QByteArray decompressedData = qUncompress(data);
    QString doc;
    QDataStream ds(&decompressedData, QIODevice::ReadOnly);
    ds >> doc;

    QDomDocument xmlDoc;
    QString error;
    int row, col;
    if (!xmlDoc.setContent(doc, true, &error, &row, &col))
    {
        DDebug() << doc << endl;
        DDebug() << error << " :: row=" << row << " , col=" << col << endl; 
        return false;
    }

    QDomElement rootElem = xmlDoc.documentElement();
    if (rootElem.tagName() != QString::fromLatin1("digikamproperties"))
        return false;

    for (QDomNode node = rootElem.firstChild();
         !node.isNull(); node = node.nextSibling()) 
    {
        QDomElement e = node.toElement();
        QString name  = e.tagName(); 
        QString val   = e.attribute(QString::fromLatin1("value")); 

        if (name == QString::fromLatin1("comments"))
        {
            comments = val;
        }    
        else if (name == QString::fromLatin1("date"))
        {
            if (val.isEmpty()) continue;
            date = QDateTime::fromString(val, Qt::ISODate);
        }
        else if (name == QString::fromLatin1("rating"))
        {
            if (val.isEmpty()) continue;
            bool ok=false;
            rating = val.toInt(&ok);
            if (!ok) rating = 0;
        }
        else if (name == QString::fromLatin1("tagslist"))
        {
            for (QDomNode node2 = e.firstChild();
                !node2.isNull(); node2 = node2.nextSibling()) 
            {
                QDomElement e2 = node2.toElement();
                QString name2  = e2.tagName(); 
                QString val2   = e2.attribute(QString::fromLatin1("path"));

                if (name2 == QString::fromLatin1("tag"))
                {
                    if (val2.isEmpty()) continue;
                    tagsPath.append(val2);
                }     
            }
        }
    }

    return true;
}

bool DMetadata::setXMLImageProperties(const QString& comments, const QDateTime& date, 
                                      int rating, const QStringList& tagsPath) const
{
    QDomDocument xmlDoc;
    
    xmlDoc.appendChild(xmlDoc.createProcessingInstruction( QString::fromLatin1("xml"),
                       QString::fromLatin1("version=\"1.0\" encoding=\"UTF-8\"") ) );

    QDomElement propertiesElem = xmlDoc.createElement(QString::fromLatin1("digikamproperties")); 
    xmlDoc.appendChild( propertiesElem );

    QDomElement c = xmlDoc.createElement(QString::fromLatin1("comments"));
    c.setAttribute(QString::fromLatin1("value"), comments);
    propertiesElem.appendChild(c);

    QDomElement d = xmlDoc.createElement(QString::fromLatin1("date"));
    d.setAttribute(QString::fromLatin1("value"), date.toString(Qt::ISODate));
    propertiesElem.appendChild(d);

    QDomElement r = xmlDoc.createElement(QString::fromLatin1("rating"));
    r.setAttribute(QString::fromLatin1("value"), rating);
    propertiesElem.appendChild(r);

    QDomElement tagsElem = xmlDoc.createElement(QString::fromLatin1("tagslist")); 
    propertiesElem.appendChild(tagsElem);

    QStringList path = tagsPath;
    for ( QStringList::Iterator it = path.begin(); it != path.end(); ++it )
    {
        QDomElement e = xmlDoc.createElement(QString::fromLatin1("tag"));
        e.setAttribute(QString::fromLatin1("path"), *it);
        tagsElem.appendChild(e);
    }

    QByteArray  data, compressedData;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds << xmlDoc.toString();
    compressedData = qCompress(data);
    return (setIptcTagData("Iptc.Application2.0x00ff", compressedData));
}

}  // NameSpace Digikam
