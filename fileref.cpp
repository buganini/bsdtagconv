/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org
    
    copyright            : (C) 2010 by Alex Novichkov
    email                : novichko@atnet.ru
                           (added APE file support)

    copyright            : (C) 2011 by Kuan-Chung Chiu
    email                : buganini@gmail.com
 ***************************************************************************/

/***************************************************************************
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License version   *
 *   2.1 as published by the Free Software Foundation.                     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful, but   *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with this library; if not, write to the Free Software   *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA         *
 *   02110-1301  USA                                                       *
 *                                                                         *
 *   Alternatively, this file is available under the Mozilla Public        *
 *   License Version 1.1.  You may obtain a copy of the License at         *
 *   http://www.mozilla.org/MPL/                                           *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <taglib/tfile.h>
#include <taglib/tstring.h>

#include "fileref.h"

#define debug(X)

using namespace TagLib;

class FileRef::FileRefPrivate : public RefCounter
{
public:
  FileRefPrivate(File *f) : RefCounter(), file(f) {}
  ~FileRefPrivate() {
    delete file;
  }

  File *file;
  static List<const FileTypeResolver *> fileTypeResolvers;
};

List<const FileRef::FileTypeResolver *> FileRef::FileRefPrivate::fileTypeResolvers;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

FileRef::FileRef()
{
  d = new FileRefPrivate(0);
}

FileRef::FileRef(FileName fileName, bool readAudioProperties,
                 AudioProperties::ReadStyle audioPropertiesStyle)
{
  filename=NULL;
  d = new FileRefPrivate(create(fileName, readAudioProperties, audioPropertiesStyle));
}

FileRef::FileRef(File *file)
{
  d = new FileRefPrivate(file);
}

FileRef::FileRef(const FileRef &ref) : d(ref.d)
{
  d->ref();
}

FileRef::~FileRef()
{
  if(d->deref())
    delete d;
}

Tag *FileRef::tag() const
{
  if(isNull()) {
    debug("FileRef::tag() - Called without a valid file.");
    return 0;
  }
  return d->file->tag();
}

AudioProperties *FileRef::audioProperties() const
{
  if(isNull()) {
    debug("FileRef::audioProperties() - Called without a valid file.");
    return 0;
  }
  return d->file->audioProperties();
}

File *FileRef::file() const
{
  return d->file;
}

bool FileRef::save()
{
  if(isNull()) {
    debug("FileRef::save() - Called without a valid file.");
    return false;
  }
  if(preferedTag()==TagType::ID3v2 || U_ID3v2.load){
    
    dynamic_cast<MPEG::File *>(d->file)->save(tags_mask(0xff), true, 3);
    return true;
  }
  return d->file->save();
}

const FileRef::FileTypeResolver *FileRef::addFileTypeResolver(const FileRef::FileTypeResolver *resolver) // static
{
  FileRefPrivate::fileTypeResolvers.prepend(resolver);
  return resolver;
}

StringList FileRef::defaultFileExtensions()
{
  StringList l;

  l.append("ogg");
  l.append("flac");
  l.append("oga");
  l.append("mp3");
  l.append("mpc");
  l.append("wv");
  l.append("spx");
  l.append("tta");
#ifdef TAGLIB_WITH_MP4
  l.append("m4a");
  l.append("m4b");
  l.append("m4p");
  l.append("3g2");
  l.append("mp4");
#endif
#ifdef TAGLIB_WITH_ASF
  l.append("wma");
  l.append("asf");
#endif
  l.append("aif");
  l.append("aiff");
  l.append("wav");
  l.append("ape");

  return l;
}

bool FileRef::isNull() const
{
  return !d->file || !d->file->isValid();
}

FileRef &FileRef::operator=(const FileRef &ref)
{
  if(&ref == this)
    return *this;

  if(d->deref())
    delete d;

  d = ref.d;
  d->ref();

  return *this;
}

bool FileRef::operator==(const FileRef &ref) const
{
  return ref.d->file == d->file;
}

bool FileRef::operator!=(const FileRef &ref) const
{
  return ref.d->file != d->file;
}

File *FileRef::create(FileName fileName, bool readAudioProperties,
                      AudioProperties::ReadStyle audioPropertiesStyle)
{

  List<const FileTypeResolver *>::ConstIterator it = FileRefPrivate::fileTypeResolvers.begin();

  for(; it != FileRefPrivate::fileTypeResolvers.end(); ++it) {
    File *file = (*it)->createFile(fileName, readAudioProperties, audioPropertiesStyle);
    if(file)
      return file;
  }

  // Ok, this is really dumb for now, but it works for testing.

  String s;

#ifdef _WIN32
  s = (wcslen((const wchar_t *) fileName) > 0) ? String((const wchar_t *) fileName) : String((const char *) fileName);
#else
  s = fileName;
#endif
  if(filename==NULL)
    filename= strdup(fileName);
  // If this list is updated, the method defaultFileExtensions() should also be
  // updated.  However at some point that list should be created at the same time
  // that a default file type resolver is created.

  int pos = s.rfind(".");
  if(pos != -1) {
    String ext = s.substr(pos + 1).upper();
    if(ext == "MP3")
    {
      filetype=FileType::MPEG;
      return mpeg = new MPEG::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "OGG")
    {
      filetype=FileType::Ogg_Vorbis;
      return ogg_vorbis = new Ogg::Vorbis::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "OGA") {
      /* .oga can be any audio in the Ogg container. First try FLAC, then Vorbis. */
      filetype=FileType::Ogg_FLAC;
      File *file = ogg_flac = new Ogg::FLAC::File(fileName, readAudioProperties, audioPropertiesStyle);
      if (file->isValid())
        return file;
      delete file;
      filetype=FileType::Ogg_Vorbis;
      return ogg_vorbis = new Ogg::Vorbis::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "FLAC")
    {
      filetype=FileType::FLAC;
      return flac = new FLAC::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "MPC")
    {
      filetype=FileType::MPC;
      return mpc = new MPC::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "WV")
    {
      filetype=FileType::WavPack;
      return wavpack = new WavPack::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "SPX")
    {
      filetype=FileType::Ogg_Speex;
      return ogg_speex = new Ogg::Speex::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "TTA")
    {
      filetype=FileType::TrueAudio;
      return trueaudio = new TrueAudio::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
#ifdef TAGLIB_WITH_MP4
    if(ext == "M4A" || ext == "M4B" || ext == "M4P" || ext == "MP4" || ext == "3G2")
    {
      filetype=FileType::MP4;
      return mp4 = new MP4::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
#endif
#ifdef TAGLIB_WITH_ASF
    if(ext == "WMA" || ext == "ASF")
    {
      filetype=FileType::ASF;
      return asf = new ASF::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
#endif
    if(ext == "AIF" || ext == "AIFF")
    {
      filetype=FileType::RIFF_AIFF;
      return riff_aiff = new RIFF::AIFF::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "WAV")
    {
      filetype=FileType::RIFF_WAV;
      return riff_wav = new RIFF::WAV::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
    if(ext == "APE")
    {
      filetype=FileType::APE;
      return ape = new APE::File(fileName, readAudioProperties, audioPropertiesStyle);
    }
  }

  return 0;
}

TagType::TagTypes FileRef::preferedTag()
{
  switch(filetype)
  {
    case FileType::FLAC:
    case FileType::MPEG:
    case FileType::RIFF_AIFF:
    case FileType::RIFF_WAV:
    case FileType::TrueAudio:
      return TagType::ID3v2;

    case FileType::Ogg_FLAC:
    case FileType::Ogg_Speex:
    case FileType::Ogg_Vorbis:
      return TagType::XiphComment;

    case FileType::APE:
    case FileType::MPC:
    case FileType::WavPack:
      return TagType::APE;

    case FileType::ASF:
      return TagType::ASF;

    default:
      return TagType::None;
  }
}

APE::Tag * FileRef::APETag(bool create)
{
  switch(filetype)
  {
    case FileType::APE:
      return ape->APETag(create);
    case FileType::MPC:
      return mpc->APETag(create);
    case FileType::MPEG:
      return mpeg->APETag(create);
    case FileType::WavPack:
      return wavpack->APETag(create);
    default:
      return NULL;
  }
}

ASF::Tag * FileRef::ASFTag(bool create)
{
  switch(filetype)
  {
    case FileType::ASF:
      if(create)
        return NULL;
      return asf->tag();
    default:
      return NULL;
  }
}

ID3v1::Tag * FileRef::ID3v1Tag (bool create)
{
  switch(filetype)
  {
    case FileType::APE:
      return ape->ID3v1Tag(create);
    case FileType::FLAC:
      return flac->ID3v1Tag(create);
    case FileType::MPC:
      return mpc->ID3v1Tag(create);
    case FileType::MPEG:
      return mpeg->ID3v1Tag(create);
    case FileType::TrueAudio:
      return trueaudio->ID3v1Tag(create);
    case FileType::WavPack:
      return wavpack->ID3v1Tag(create);
    default:
      return NULL;
  }
}

ID3v2::Tag * FileRef::ID3v2Tag (bool create)
{
  switch(filetype)
  {
    case FileType::FLAC:
      return flac->ID3v2Tag(create);
    case FileType::MPEG:
      return mpeg->ID3v2Tag(create);
    case FileType::TrueAudio:
      return trueaudio->ID3v2Tag(create);
    default:
      return NULL;
  }
}

MP4::Tag * FileRef::MP4Tag (bool create)
{
  switch(filetype)
  {
    case FileType::MP4:
      if(create)
        return NULL;
      return mp4->tag();
    default:
      return NULL;
  }
}

Ogg::XiphComment * FileRef::XiphComment (bool create)
{
  switch(filetype)
  {
    case FileType::Ogg_FLAC:
      if(create)
        return NULL;
      return ogg_flac->tag();
    case FileType::Ogg_Speex:
      if(create)
        return NULL;
      return ogg_speex->tag();
    case FileType::Ogg_Vorbis:
      if(create)
        return NULL;
      return ogg_vorbis->tag();
    default:
      return NULL;
  }
}

Tag * FileRef::anyTag (bool create)
{
  switch(filetype)
  {
    case FileType::MP4:
      return mp4->tag();
    default:
      return NULL;
  }
}

bool FileRef::strip (int tags)
{
  if(tags & TagType::APE) U_APE.load=false;
  if(tags & TagType::ID3v1) U_ID3v1.load=false;
  if(tags & TagType::ID3v2) U_ID3v2.load=false;
  if(tags & TagType::MP4) U_MP4.load=false;
  if(tags & TagType::XiphComment) U_Xiph.load=false;
  tags=tags_mask(tags);
  switch(filetype)
  {
    case FileType::APE:
      ape->strip(tags);
      return true;
    case FileType::ASF:
      return false;
    case FileType::FLAC:
      return false;
    case FileType::MP4:
      return false;
    case FileType::MPC:
      mpc->strip(tags);
      return true;
    case FileType::MPEG:
      return mpeg->strip(tags);
    case FileType::Ogg_FLAC:
      return false;
    case FileType::Ogg_Speex:
      return false;
    case FileType::Ogg_Vorbis:
      return false;
    case FileType::RIFF_AIFF:
      return false;
    case FileType::RIFF_WAV:
      return false;
    case FileType::TrueAudio:
      trueaudio->strip(tags);
      return true;
    case FileType::WavPack:
      wavpack->strip(tags);
      return true;
    default:
      return false;
  }
}

int FileRef::tags_mask (int tags)
{
  int ret=0;
  switch(filetype)
  {
    case FileType::APE:
      if(tags & TagType::APE) ret|=APE::File::APE;
      if(tags & TagType::ID3v1) ret|=APE::File::ID3v1;
      return ret;
    case FileType::ASF:
      return ret;
    case FileType::FLAC:
      return ret;
    case FileType::MP4:
      return ret;
    case FileType::MPC:
      if(tags & TagType::APE) ret|=MPC::File::APE;
      if(tags & TagType::ID3v1) ret|=MPC::File::ID3v1;
      if(tags & TagType::ID3v2) ret|=MPC::File::ID3v2;
      return ret;
    case FileType::MPEG:
      if(tags & TagType::APE) ret|=MPEG::File::APE;
      if(tags & TagType::ID3v1) ret|=MPEG::File::ID3v1;
      if(tags & TagType::ID3v2) ret|=MPEG::File::ID3v2;
      return ret;
    case FileType::Ogg_FLAC:
      return ret;
    case FileType::Ogg_Speex:
      return ret;
    case FileType::Ogg_Vorbis:
      return ret;
    case FileType::RIFF_AIFF:
      return ret;
    case FileType::RIFF_WAV:
      return ret;
    case FileType::TrueAudio:
      if(tags & TagType::ID3v1) ret|=TrueAudio::File::ID3v1;
      if(tags & TagType::ID3v2) ret|=TrueAudio::File::ID3v2;
      return ret;
    case FileType::WavPack:
      if(tags & TagType::APE) ret|=WavPack::File::APE;
      if(tags & TagType::ID3v1) ret|=WavPack::File::ID3v1;
      return ret;
    default:
      return 0;
  }
}

UniTag::UniTag()
{
  load=false;
  title=String::null;
  artist=String::null;
  album=String::null;
  comment=String::null;
  genre=String::null;
  rating=String::null;
  copyright=String::null;
}

UniTag& UniTag::operator=(const APE::Tag * APETag)
{
  if(APETag==NULL)
    return *this;

  load=true;

  title=APETag->title();
  if(title.isEmpty()) title=String::null;

  artist=APETag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=APETag->album();
  if(album.isEmpty()) album=String::null;

  comment=APETag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=APETag->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const ASF::Tag * ASFTag)
{
  if(ASFTag==NULL)
    return *this;

  load=true;

  title=ASFTag->title();
  if(title.isEmpty()) title=String::null;

  artist=ASFTag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=ASFTag->album();
  if(album.isEmpty()) album=String::null;

  comment=ASFTag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=ASFTag->genre();
  if(genre.isEmpty()) genre=String::null;

  rating=ASFTag->rating();
  if(rating.isEmpty()) rating=String::null;

  copyright=ASFTag->copyright();
  if(copyright.isEmpty()) copyright=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull() &&
     rating.isNull() &&
     copyright.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const ID3v1::Tag * ID3v1Tag)
{
  if(ID3v1Tag==NULL)
    return *this;

  load=true;

  title=ID3v1Tag->title();
  if(title.isEmpty()) title=String::null;

  artist=ID3v1Tag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=ID3v1Tag->album();
  if(album.isEmpty()) album=String::null;

  comment=ID3v1Tag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=ID3v1Tag->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const ID3v2::Tag * ID3v2Tag)
{
  if(ID3v2Tag==NULL)
    return *this;

  load=true;

  title=ID3v2Tag->title();
  if(title.isEmpty()) title=String::null;

  artist=ID3v2Tag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=ID3v2Tag->album();
  if(album.isEmpty()) album=String::null;

  comment=ID3v2Tag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=ID3v2Tag->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const MP4::Tag * MP4Tag)
{
  if(MP4Tag==NULL)
    return *this;

  load=true;

  title=MP4Tag->title();
  if(title.isEmpty()) title=String::null;

  artist=MP4Tag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=MP4Tag->album();
  if(album.isEmpty()) album=String::null;

  comment=MP4Tag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=MP4Tag->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const Ogg::XiphComment * XiphComment)
{
  if(XiphComment==NULL)
    return *this;

  load=true;

  title=XiphComment->title();
  if(title.isEmpty()) title=String::null;

  artist=XiphComment->artist();
  if(artist.isEmpty()) artist=String::null;

  album=XiphComment->album();
  if(album.isEmpty()) album=String::null;

  comment=XiphComment->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=XiphComment->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}

UniTag& UniTag::operator=(const Tag * Tag)
{
  if(Tag==NULL)
    return *this;

  load=true;

  title=Tag->title();
  if(title.isEmpty()) title=String::null;

  artist=Tag->artist();
  if(artist.isEmpty()) artist=String::null;

  album=Tag->album();
  if(album.isEmpty()) album=String::null;

  comment=Tag->comment();
  if(comment.isEmpty()) comment=String::null;

  genre=Tag->genre();
  if(genre.isEmpty()) genre=String::null;

  if(title.isNull() &&
     artist.isNull() &&
     album.isNull() &&
     comment.isNull() &&
     genre.isNull()){
     load=false;
  }
  return *this;
}
