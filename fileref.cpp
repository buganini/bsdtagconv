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

#include <id3/tag.h>

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

unicode_t * String2Unicode_t(String str){
    ByteVector bv=str.data(String::UTF16LE);
    char *p;
    int i,i2,l;
    p=bv.data();
    l=bv.size();
    unicode_t *ret=new unicode_t[l/2+1];
    for(i=0,i2=0;i2<l;++i,i2+=2){
      ret[i]=((unsigned char)p[i2] << 8) | (unsigned char)p[i2+1];
    }
    ret[i]=0;
    return ret;
}

bool FileRef::save()
{
  if(isNull()) {
    debug("FileRef::save() - Called without a valid file.");
    return false;
  }
  if(preferedTag()==TagType::ID3v2 || U_ID3v2.load){
    ID3_Tag id3t;
    ID3_Frame *frame;
    unicode_t *p;

    frame=new ID3_Frame(ID3FID_TITLE);
    frame->GetField(ID3FN_TEXT)->SetEncoding(ID3TE_UNICODE);
    frame->GetField(ID3FN_TEXT)->Set(p=String2Unicode_t(U_ID3v2.title));
    frame->GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
    id3t.AttachFrame(frame);
    delete p;

    frame=new ID3_Frame(ID3FID_LEADARTIST);
    frame->GetField(ID3FN_TEXT)->SetEncoding(ID3TE_UNICODE);
    frame->GetField(ID3FN_TEXT)->Set(p=String2Unicode_t(U_ID3v2.artist));
    frame->GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
    id3t.AttachFrame(frame);
    delete p;

    frame=new ID3_Frame(ID3FID_ALBUM);
    frame->GetField(ID3FN_TEXT)->SetEncoding(ID3TE_UNICODE);
    frame->GetField(ID3FN_TEXT)->Set(p=String2Unicode_t(U_ID3v2.album));
    frame->GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
    id3t.AttachFrame(frame);
    delete p;

    frame=new ID3_Frame(ID3FID_COMMENT);
    frame->GetField(ID3FN_TEXT)->SetEncoding(ID3TE_UNICODE);
    frame->GetField(ID3FN_TEXT)->Set(p=String2Unicode_t(U_ID3v2.comment));
    frame->GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
    id3t.AttachFrame(frame);
    delete p;

    frame=new ID3_Frame(ID3FID_CONTENTTYPE);
    frame->GetField(ID3FN_TEXT)->SetEncoding(ID3TE_UNICODE);
    frame->GetField(ID3FN_TEXT)->Set(p=String2Unicode_t(U_ID3v2.genre));
    frame->GetField(ID3FN_TEXTENC)->Set(ID3TE_UNICODE);
    id3t.AttachFrame(frame);
    delete p;

    d->file->save();
    delete d;
    id3t.Link(filename, ID3TT_NONE);
    id3t.Update(ID3TT_ID3V2);
    id3t.Clear();
    d = new FileRefPrivate(create(filename));
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
  if(tags==0xffff){
    U_APE.load=false;
    U_ASF.load=false;
    U_ID3v1.load=false;
    U_ID3v2.load=false;
    U_MP4.load=false;
    U_Xiph.load=false;
  }
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
  load=true;
  title=APETag->title();
  artist=APETag->artist();
  album=APETag->album();
  comment=APETag->comment();
  genre=APETag->genre();
  return *this;
}

UniTag& UniTag::operator=(const ASF::Tag * ASFTag)
{
  load=true;
  title=ASFTag->title();
  artist=ASFTag->artist();
  album=ASFTag->album();
  comment=ASFTag->comment();
  genre=ASFTag->genre();
  rating=ASFTag->rating();
  copyright=ASFTag->copyright();
  return *this;
}

UniTag& UniTag::operator=(const ID3v1::Tag * ID3v1Tag)
{
  load=true;
  title=ID3v1Tag->title();
  artist=ID3v1Tag->artist();
  album=ID3v1Tag->album();
  comment=ID3v1Tag->comment();
  genre=ID3v1Tag->genre();
  return *this;
}

UniTag& UniTag::operator=(const ID3v2::Tag * ID3v2Tag)
{
  load=true;
  title=ID3v2Tag->title();
  artist=ID3v2Tag->artist();
  album=ID3v2Tag->album();
  comment=ID3v2Tag->comment();
  genre=ID3v2Tag->genre();
  return *this;
}

UniTag& UniTag::operator=(const MP4::Tag * MP4Tag)
{
  load=true;
  title=MP4Tag->title();
  artist=MP4Tag->artist();
  album=MP4Tag->album();
  comment=MP4Tag->comment();
  genre=MP4Tag->genre();
  return *this;
}

UniTag& UniTag::operator=(const Ogg::XiphComment * XiphComment)
{
  load=true;
  title=XiphComment->title();
  artist=XiphComment->artist();
  album=XiphComment->album();
  comment=XiphComment->comment();
  genre=XiphComment->genre();
  return *this;
}

UniTag& UniTag::operator=(const Tag * Tag)
{
  load=true;
  title=Tag->title();
  artist=Tag->artist();
  album=Tag->album();
  comment=Tag->comment();
  genre=Tag->genre();
  return *this;
}
