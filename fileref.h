/***************************************************************************
    copyright            : (C) 2002 - 2008 by Scott Wheeler
    email                : wheeler@kde.org

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

#ifndef TAGLIB_FILEREF_H
#define TAGLIB_FILEREF_H

#include <taglib/tfile.h>
#include <taglib/tstringlist.h>

#include <taglib/asffile.h>
#include <taglib/mpegfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/flacfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/mpcfile.h>
#include <taglib/mp4file.h>
#include <taglib/wavpackfile.h>
#include <taglib/speexfile.h>
#include <taglib/trueaudiofile.h>
#include <taglib/aifffile.h>
#include <taglib/wavfile.h>
#include <taglib/apefile.h>

#include <taglib/apetag.h>
#include <taglib/asftag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4tag.h>
#include <taglib/xiphcomment.h>

#include <taglib/taglib_export.h>
#include <taglib/audioproperties.h>

namespace TagLib {

  class Tag;

  //! This class provides a simple abstraction for creating and handling files

  /*!
   * FileRef exists to provide a minimal, generic and value-based wrapper around
   * a File.  It is lightweight and implicitly shared, and as such suitable for
   * pass-by-value use.  This hides some of the uglier details of TagLib::File
   * and the non-generic portions of the concrete file implementations.
   *
   * This class is useful in a "simple usage" situation where it is desirable
   * to be able to get and set some of the tag information that is similar
   * across file types.
   *
   * Also note that it is probably a good idea to plug this into your mime
   * type system rather than using the constructor that accepts a file name using
   * the FileTypeResolver.
   *
   * \see FileTypeResolver
   * \see addFileTypeResolver()
   */
  class FileType
  {
    public:
    enum FileTypes
    {
      APE,
      ASF,
      FLAC,
      MP4,
      MPC,
      MPEG,
      Ogg_FLAC,
      Ogg_Speex,
      Ogg_Vorbis,
      RIFF_AIFF,
      RIFF_WAV,
      TrueAudio,
      WavPack,
    };
  };

  class TagType
  {
    public:
    enum TagTypes
    {
      None=0,
      APE=1,
      ASF=2,
      ID3v1=4,
      ID3v2=8,
      MP4=16,
      XiphComment=32,
      Tag=64
    };
  };

  class UniTag {
    public:
    bool load;
    TagLib::String title;
    TagLib::String artist;
    TagLib::String album;
    TagLib::String comment;
    TagLib::String genre;
    TagLib::String rating;
    TagLib::String copyright;
    UniTag();
    UniTag &operator=(const APE::Tag *);
    UniTag &operator=(const ASF::Tag *);
    UniTag &operator=(const ID3v1::Tag *);
    UniTag &operator=(const ID3v2::Tag *);
    UniTag &operator=(const MP4::Tag *);
    UniTag &operator=(const Ogg::XiphComment *);
    UniTag &operator=(const Tag *);
  };

  class TAGLIB_EXPORT FileRef
  {
  public:

  //! A class for pluggable file type resolution.

  /*!
   * This class is used to add extend TagLib's very basic file name based file
   * type resolution.
   *
   * This can be accomplished with:
   *
   * \code
   *
   * class MyFileTypeResolver : FileTypeResolver
   * {
   *   TagLib::File *createFile(TagLib::FileName *fileName, bool, AudioProperties::ReadStyle)
   *   {
   *     if(someCheckForAnMP3File(fileName))
   *       return new TagLib::MPEG::File(fileName);
   *     return 0;
   *   }
   * }
   *
   * FileRef::addFileTypeResolver(new MyFileTypeResolver);
   *
   * \endcode
   *
   * Naturally a less contrived example would be slightly more complex.  This
   * can be used to plug in mime-type detection systems or to add new file types
   * to TagLib.
   */

    class TAGLIB_EXPORT FileTypeResolver
    {
      TAGLIB_IGNORE_MISSING_DESTRUCTOR
    public:
      /*!
       * This method must be overridden to provide an additional file type
       * resolver.  If the resolver is able to determine the file type it should
       * return a valid File object; if not it should return 0.
       *
       * \note The created file is then owned by the FileRef and should not be
       * deleted.  Deletion will happen automatically when the FileRef passes
       * out of scope.
       */
      virtual File *createFile(FileName fileName,
                               bool readAudioProperties = true,
                               AudioProperties::ReadStyle
                               audioPropertiesStyle = AudioProperties::Average) const = 0;
    };

    /*!
     * Creates a null FileRef.
     */
    FileRef();

    /*!
     * Create a FileRef from \a fileName.  If \a readAudioProperties is true then
     * the audio properties will be read using \a audioPropertiesStyle.  If
     * \a readAudioProperties is false then \a audioPropertiesStyle will be
     * ignored.
     *
     * Also see the note in the class documentation about why you may not want to
     * use this method in your application.
     */
    explicit FileRef(FileName fileName,
                     bool readAudioProperties = true,
                     AudioProperties::ReadStyle
                     audioPropertiesStyle = AudioProperties::Average);

    /*!
     * Contruct a FileRef using \a file.  The FileRef now takes ownership of the
     * pointer and will delete the File when it passes out of scope.
     */
    explicit FileRef(File *file);

    /*!
     * Make a copy of \a ref.
     */
    FileRef(const FileRef &ref);

    /*!
     * Destroys this FileRef instance.
     */
    virtual ~FileRef();

    /*!
     * Returns a pointer to represented file's tag.
     *
     * \warning This pointer will become invalid when this FileRef and all
     * copies pass out of scope.
     *
     * \warning Do not cast it to any subclasses of \class Tag.
     * Use tag returning methods of appropriate subclasses of \class File instead.
     *
     * \see File::tag()
     */
    Tag *tag() const;

    /*!
     * Returns the audio properties for this FileRef.  If no audio properties
     * were read then this will returns a null pointer.
     */
    AudioProperties *audioProperties() const;

    /*!
     * Returns a pointer to the file represented by this handler class.
     *
     * As a general rule this call should be avoided since if you need to work
     * with file objects directly, you are probably better served instantiating
     * the File subclasses (i.e. MPEG::File) manually and working with their APIs.
     *
     * This <i>handle</i> exists to provide a minimal, generic and value-based
     * wrapper around a File.  Accessing the file directly generally indicates
     * a moving away from this simplicity (and into things beyond the scope of
     * FileRef).
     *
     * \warning This pointer will become invalid when this FileRef and all
     * copies pass out of scope.
     */
    File *file() const;

    /*!
     * Saves the file.  Returns true on success.
     */
    bool save();

    /*!
     * Adds a FileTypeResolver to the list of those used by TagLib.  Each
     * additional FileTypeResolver is added to the front of a list of resolvers
     * that are tried.  If the FileTypeResolver returns zero the next resolver
     * is tried.
     *
     * Returns a pointer to the added resolver (the same one that's passed in --
     * this is mostly so that static inialializers have something to use for
     * assignment).
     *
     * \see FileTypeResolver
     */
    static const FileTypeResolver *addFileTypeResolver(const FileTypeResolver *resolver);

    /*!
     * As is mentioned elsewhere in this class's documentation, the default file
     * type resolution code provided by TagLib only works by comparing file
     * extensions.
     *
     * This method returns the list of file extensions that are used by default.
     *
     * The extensions are all returned in lowercase, though the comparison used
     * by TagLib for resolution is case-insensitive.
     *
     * \note This does not account for any additional file type resolvers that
     * are plugged in.  Also note that this is not intended to replace a propper
     * mime-type resolution system, but is just here for reference.
     *
     * \see FileTypeResolver
     */
    static StringList defaultFileExtensions();

    /*!
     * Returns true if the file (and as such other pointers) are null.
     */
    bool isNull() const;

    /*!
     * Assign the file pointed to by \a ref to this FileRef.
     */
    FileRef &operator=(const FileRef &ref);

    /*!
     * Returns true if this FileRef and \a ref point to the same File object.
     */
    bool operator==(const FileRef &ref) const;

    /*!
     * Returns true if this FileRef and \a ref do not point to the same File
     * object.
     */
    bool operator!=(const FileRef &ref) const;

    /*!
     * A simple implementation of file type guessing.  If \a readAudioProperties
     * is true then the audio properties will be read using
     * \a audioPropertiesStyle.  If \a readAudioProperties is false then
     * \a audioPropertiesStyle will be ignored.
     *
     * \note You generally shouldn't use this method, but instead the constructor
     * directly.
     *
     * \deprecated
     */
    File *create(FileName fileName,
                        bool readAudioProperties = true,
                        AudioProperties::ReadStyle audioPropertiesStyle = AudioProperties::Average);

    TagType::TagTypes preferedTag();
    APE::Tag * APETag(bool create=false);
    ASF::Tag * ASFTag(bool create=false);
    ID3v1::Tag * ID3v1Tag (bool create=false);
    ID3v2::Tag * ID3v2Tag (bool create=false);
    MP4::Tag * MP4Tag (bool create=false);
    Ogg::XiphComment * XiphComment (bool create=false);
    Tag * anyTag (bool create=false);

    int tags_mask(int);
    bool strip(int tags);

    char *filename;

    APE::File *ape;
    ASF::File *asf;
    FLAC::File *flac;
    MP4::File *mp4;
    MPC::File *mpc;
    MPEG::File *mpeg;
    Ogg::FLAC::File *ogg_flac;
    Ogg::Speex::File *ogg_speex;
    Ogg::Vorbis::File *ogg_vorbis;
    RIFF::AIFF::File *riff_aiff;
    RIFF::WAV::File *riff_wav;
    TrueAudio::File *trueaudio;
    WavPack::File *wavpack;

    FileType::FileTypes filetype;
    UniTag U_APE, U_ASF, U_ID3v1, U_ID3v2, U_MP4, U_Xiph, U_Tag;

  private:
    class FileRefPrivate;
    FileRefPrivate *d;
  };

} // namespace TagLib

#endif
