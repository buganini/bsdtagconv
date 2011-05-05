/*
 * Copyright (c) 2011 Kuan-Chung Chiu <buganini@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <iostream>
#include <string>
#include <taglib/tbytevector.h>
#include <taglib/tag.h>
#include <taglib/apetag.h>
#include <taglib/asftag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/tstring.h>
#include <bsdconv.h>
#include "fileref.h"

enum field{
	TITLE,
	ARTIST,
	ALBUM,
	COMMENT,
	GENRE
};

using namespace std;

int convn;
int testarg,skiparg,skip,autoarg,eachconv,strip;
int force_decode_all;
int force_decode_ape;
int force_decode_asf;
int force_decode_id3v2;
int force_decode_mp4;
int force_decode_xiph;

struct bsdconv_instance **convs;
struct bsdconv_instance *inter;
struct bsdconv_instance *score;
struct bsdconv_instance *render;
int *scores;
int bestCodec;

class ID3v1StringHandler : public TagLib::ID3v1::StringHandler
{
	virtual TagLib::ByteVector render( const TagLib::String &s ) const
	{
		if(::render==NULL)
			return TagLib::ByteVector("", 0);
		TagLib::ByteVector bv(s.to8Bit(true).c_str());
		bsdconv_init(::render);
		::render->input.data=(void *)bv.data();
		::render->input.len=bv.size();
		::render->input.flags=0;
		::render->flush=1;
		::render->output_mode=BSDCONV_AUTOMALLOC;
		::render->output.len=1;
		bsdconv(::render);
		TagLib::ByteVector ret=TagLib::ByteVector((char *)::render->output.data, ::render->output.len);
		free(::render->output.data);
		return ret;		
	}
};

void autoconv_init(){
	int i;
	for(i=0;i<convn;++i){
		scores[i]=0;
	}
}

void autoconv_test(TagLib::String s){
	TagLib::ByteVector bv(s.to8Bit(true).c_str());
	int i,max;
	struct bsdconv_instance *ins=NULL;
	for(i=0;i<convn;++i){
		ins=convs[i];
		bsdconv_init(ins);
		ins->output_mode=BSDCONV_HOLD;
		ins->input.data=(void *)bv.data();
		ins->input.len=bv.size();
		ins->input.flags=0;
		ins->flush=1;
		bsdconv(ins);
		scores[i]+=ins->score + ins->ierr*(-3) + ins->oerr*(-2);
	}
	max=0;
	for(i=0;i<convn;++i){
		if(scores[i]>scores[max]){
			max=i;
		}
	}
	bestCodec=max;
}

TagLib::String autoconv(TagLib::String s){
	TagLib::ByteVector bv(s.to8Bit(true).c_str());
	struct bsdconv_instance *ins=NULL;

	ins=convs[bestCodec];
	bsdconv_init(ins);
	ins->input.data=(void *)bv.data();
	ins->input.len=bv.size();
	ins->input.flags=0;
	ins->flush=1;
	ins->output_mode=BSDCONV_AUTOMALLOC;
	ins->output.len=1;
	bsdconv(ins);
	((char *)ins->output.data)[ins->output.len]=0;
	if(ins->ierr || ins->oerr){
		skip=1;
	}else{
		skip=0;
	}
	TagLib::String ret((const char *)ins->output.data, TagLib::String::UTF8);
	free(ins->output.data);
	return ret;
}

int score_eval(TagLib::String s){
	int ret;
	TagLib::ByteVector bv(s.to8Bit(true).c_str());
	bsdconv_init(score);
	score->output_mode=BSDCONV_HOLD;
	score->input.data=(void *)bv.data();
	score->input.len=bv.size();
	score->input.flags=0;
	score->flush=1;
	bsdconv(score);
	ret=score->score + score->ierr*(-3) + score->oerr*(-2);
	return ret;
}

void autoConv(TagLib::UniTag &U){
	if(eachconv){
		autoconv_init();
		if(!U.title.isNull()) autoconv_test(U.title);
		if(!U.title.isNull()) U.title=autoconv(U.title);

		autoconv_init();
		if(!U.artist.isNull()) autoconv_test(U.artist);
		if(!U.artist.isNull()) U.artist=autoconv(U.artist);

		autoconv_init();
		if(!U.album.isNull()) autoconv_test(U.album);
		if(!U.album.isNull()) U.album=autoconv(U.album);

		autoconv_init();
		if(!U.comment.isNull()) autoconv_test(U.comment);
		if(!U.comment.isNull()) U.comment=autoconv(U.comment);

		autoconv_init();
		if(!U.genre.isNull()) autoconv_test(U.genre);
		if(!U.genre.isNull()) U.genre=autoconv(U.genre);

		autoconv_init();
		if(!U.rating.isNull()) autoconv_test(U.rating);
		if(!U.rating.isNull()) U.genre=autoconv(U.rating);

		autoconv_init();
		if(!U.copyright.isNull()) autoconv_test(U.copyright);
		if(!U.copyright.isNull()) U.genre=autoconv(U.copyright);

	}else{
		autoconv_init();

		if(!U.title.isNull()) autoconv_test(U.title);
		if(!U.artist.isNull()) autoconv_test(U.artist);
		if(!U.album.isNull()) autoconv_test(U.album);
		if(!U.comment.isNull()) autoconv_test(U.comment);
		if(!U.genre.isNull()) autoconv_test(U.genre);
		if(!U.rating.isNull()) autoconv_test(U.rating);
		if(!U.copyright.isNull()) autoconv_test(U.copyright);

		if(!U.title.isNull()) U.title=autoconv(U.title);
		if(!U.artist.isNull()) U.artist=autoconv(U.artist);
		if(!U.album.isNull()) U.album=autoconv(U.album);
		if(!U.comment.isNull()) U.comment=autoconv(U.comment);
		if(!U.genre.isNull()) U.genre=autoconv(U.genre);
		if(!U.rating.isNull()) U.genre=autoconv(U.rating);
		if(!U.copyright.isNull()) U.genre=autoconv(U.copyright);
	}
}

TagLib::String conv(TagLib::String res){
	TagLib::String ret=res;
	if(inter){
		TagLib::ByteVector bv(res.to8Bit(true).c_str());
		bsdconv_init(inter);
		inter->input.data=bv.data();
		inter->input.len=bv.size();
		inter->input.flags=0;
		inter->flush=1;
		inter->output_mode=BSDCONV_AUTOMALLOC;
		inter->output.len=1;
		bsdconv(inter);
		((char *)inter->output.data)[inter->output.len]=0;
		ret=TagLib::String((const char *)inter->output.data, TagLib::String::UTF8);
		free(inter->output.data);
	}
	if(skip!=0 && skiparg!=0){
		ret=res;
	}
	return ret;

}

void Conv(TagLib::UniTag &U){
	if(!U.title.isNull()) U.title=conv(U.title);
	if(!U.artist.isNull()) U.artist=conv(U.artist);
	if(!U.album.isNull()) U.album=conv(U.album);
	if(!U.comment.isNull()) U.comment=conv(U.comment);
	if(!U.genre.isNull()) U.genre=conv(U.genre);
	if(!U.rating.isNull()) U.genre=conv(U.rating);
	if(!U.copyright.isNull()) U.genre=conv(U.copyright);
}

void printUniTag(TagLib::UniTag &U){
	if(!U.title.isNull()) cout << "\t\tTitle: " << U.title.to8Bit(true) << endl;
	if(!U.artist.isNull()) cout << "\t\tArtist: " << U.artist.to8Bit(true) << endl;
	if(!U.album.isNull()) cout << "\t\tAlbum: " << U.album.to8Bit(true) << endl;
	if(!U.comment.isNull()) cout << "\t\tComment: " << U.comment.to8Bit(true) << endl;
	if(!U.genre.isNull()) cout << "\t\tGenre: " << U.genre.to8Bit(true) << endl;
	if(!U.rating.isNull()) cout << "\t\tRating: " << U.rating.to8Bit(true) << endl;
	if(!U.copyright.isNull()) cout << "\t\tCopyright: " << U.copyright.to8Bit(true) << endl;
}

void better(TagLib::UniTag &A, TagLib::UniTag &B){
	if(score_eval(A.title) < score_eval(B.title)) A.title=B.title;
	if(score_eval(A.artist) < score_eval(B.artist)) A.artist=B.artist;
	if(score_eval(A.album) < score_eval(B.album)) A.album=B.album;
	if(score_eval(A.comment) < score_eval(B.comment)) A.comment=B.comment;
	if(score_eval(A.genre) < score_eval(B.genre)) A.genre=B.genre;
	if(score_eval(A.rating) < score_eval(B.rating)) A.rating=B.rating;
	if(score_eval(A.copyright) < score_eval(B.copyright)) A.copyright=B.copyright;
}

int proc(char *file){
	TagLib::APE::Tag * APETag=NULL;
	TagLib::ASF::Tag * ASFTag=NULL;
	TagLib::ID3v1::Tag * ID3v1Tag=NULL;
	TagLib::ID3v2::Tag * ID3v2Tag=NULL;
	TagLib::MP4::Tag * MP4Tag=NULL;
	TagLib::Ogg::XiphComment * XiphComment=NULL;
	TagLib::Tag * Tag=NULL;
	TagLib::FileRef f(file);
	if(!f.isNull()){
		cout << file << endl;
		if(f.APETag()){
			APETag=f.APETag(false);
			f.U_APE=APETag;
			if(force_decode_ape)
				autoConv(f.U_APE);
			Conv(f.U_APE);
		}
		if(f.ASFTag()){
			ASFTag=f.ASFTag(false);
			f.U_ASF=ASFTag;
			if(force_decode_asf)
				autoConv(f.U_ASF);
			Conv(f.U_ASF);
		}
		if(f.ID3v1Tag()){
			ID3v1Tag=f.ID3v1Tag(false);
			f.U_ID3v1=ID3v1Tag;
			autoConv(f.U_ID3v1);
			Conv(f.U_ID3v1);
		}
		if(f.ID3v2Tag()){
			ID3v2Tag=f.ID3v2Tag(false);
			f.U_ID3v2=ID3v2Tag;
			if(force_decode_id3v2)
				autoConv(f.U_ID3v2);
			Conv(f.U_ID3v2);
		}
		if(f.MP4Tag()){
			MP4Tag=f.MP4Tag(false);
			f.U_MP4=MP4Tag;
			if(force_decode_mp4)
				autoConv(f.U_MP4);
			Conv(f.U_MP4);
		}
		if(f.XiphComment()){
			XiphComment=f.XiphComment(false);
			f.U_Xiph=XiphComment;
			if(force_decode_xiph)
				autoConv(f.U_Xiph);
			Conv(f.U_Xiph);
		}
		if(f.anyTag()){
			Tag=f.anyTag();
			f.U_Tag=Tag;
			if(force_decode_all)
				autoConv(f.U_Tag);
			Conv(f.U_Tag);
		}
		if(autoarg){
			better(f.U_Tag, f.U_APE);
			better(f.U_Tag, f.U_ASF);
			better(f.U_Tag, f.U_ID3v1);
			better(f.U_Tag, f.U_ID3v2);
			better(f.U_Tag, f.U_MP4);
			better(f.U_Tag, f.U_Xiph);
		}
		if(!strip && f.APETag()){
			cout << "\tAPE Tag:" << endl;
			printUniTag(f.U_APE);
		}
		if(!strip && f.ASFTag()){
			cout << "\tASF Tag:" << endl;
			printUniTag(f.U_ASF);		
		}
		if(!strip && f.ID3v1Tag()){
			cout << "\tID3v1 Tag:" << endl;
			printUniTag(f.U_ID3v1);
		}
		if(!strip && f.ID3v2Tag()){
			cout << "\tID3v2 Tag:" << endl;
			printUniTag(f.U_ID3v2);
		}
		if(!strip && f.MP4Tag()){
			cout << "\tMP4 Tag:" << endl;
			printUniTag(f.U_MP4);
		}
		if(!strip && f.XiphComment()){
			cout << "\tXiphComment Tag:" << endl;
			printUniTag(f.U_Xiph);
		}
		if(!strip && f.anyTag()){
			cout << "\tTag:" << endl;
			printUniTag(f.U_Tag);
		}
		if(!testarg && autoarg && strip){
			f.strip(0xffff);
			f.save();
		}
		if(f.U_APE.load){
			APETag->setTitle(f.U_APE.title);
			APETag->setArtist(f.U_APE.artist);
			APETag->setAlbum(f.U_APE.album);
			APETag->setComment(f.U_APE.comment);
			APETag->setGenre(f.U_APE.genre);
		}
		if(f.U_ASF.load){
			ASFTag->setTitle(f.U_ASF.title);
			ASFTag->setArtist(f.U_ASF.artist);
			ASFTag->setAlbum(f.U_ASF.album);
			ASFTag->setComment(f.U_ASF.comment);
			ASFTag->setGenre(f.U_ASF.genre);
			ASFTag->setRating(f.U_ASF.rating);
			ASFTag->setCopyright(f.U_ASF.copyright);
		}
		if(f.U_ID3v1.load){
			ID3v1Tag->setTitle(f.U_ID3v1.title);
			ID3v1Tag->setArtist(f.U_ID3v1.artist);
			ID3v1Tag->setAlbum(f.U_ID3v1.album);
			ID3v1Tag->setComment(f.U_ID3v1.comment);
			ID3v1Tag->setGenre(f.U_ID3v1.genre);
		}
		if(f.U_ID3v2.load){
			ID3v2Tag->setTitle(f.U_ID3v2.title);
			ID3v2Tag->setArtist(f.U_ID3v2.artist);
			ID3v2Tag->setAlbum(f.U_ID3v2.album);
			ID3v2Tag->setComment(f.U_ID3v2.comment);
			ID3v2Tag->setGenre(f.U_ID3v2.genre);
		}
		if(f.U_MP4.load){
			MP4Tag->setTitle(f.U_MP4.title);
			MP4Tag->setArtist(f.U_MP4.artist);
			MP4Tag->setAlbum(f.U_MP4.album);
			MP4Tag->setComment(f.U_MP4.comment);
			MP4Tag->setGenre(f.U_MP4.genre);
		}
		if(f.U_Xiph.load){
			XiphComment->setTitle(f.U_Xiph.title);
			XiphComment->setArtist(f.U_Xiph.artist);
			XiphComment->setAlbum(f.U_Xiph.album);
			XiphComment->setComment(f.U_Xiph.comment);
			XiphComment->setGenre(f.U_Xiph.genre);
		}
		if(autoarg){
			f.U_Tag.load=true;
			switch(f.preferedTag()){
				case TagLib::TagType::None:
					cout << "\tpreferedType: None" << endl;
					break;
				case TagLib::TagType::APE:
					cout << "\tpreferedType: APE" << endl;
					f.U_APE=f.U_Tag;
					APETag=f.APETag(true);
					APETag->setTitle(f.U_Tag.title);
					APETag->setArtist(f.U_Tag.artist);
					APETag->setAlbum(f.U_Tag.album);
					APETag->setComment(f.U_Tag.comment);
					APETag->setGenre(f.U_Tag.genre);
					break;
				case TagLib::TagType::ASF:
					cout << "\tpreferedType: ASF" << endl;
					f.U_ASF=f.U_Tag;
					ASFTag=f.ASFTag(true);
					ASFTag->setTitle(f.U_Tag.title);
					ASFTag->setArtist(f.U_Tag.artist);
					ASFTag->setAlbum(f.U_Tag.album);
					ASFTag->setComment(f.U_Tag.comment);
					ASFTag->setGenre(f.U_Tag.genre);
					ASFTag->setRating(f.U_Tag.rating);
					ASFTag->setCopyright(f.U_Tag.copyright);
					break;
				case TagLib::TagType::ID3v1:
					f.U_ID3v1=f.U_Tag;
					cout << "\tpreferedType: ID3v1" << endl;
					ID3v1Tag=f.ID3v1Tag(true);
					ID3v1Tag->setTitle(f.U_Tag.title);
					ID3v1Tag->setArtist(f.U_Tag.artist);
					ID3v1Tag->setAlbum(f.U_Tag.album);
					ID3v1Tag->setComment(f.U_Tag.comment);
					ID3v1Tag->setGenre(f.U_Tag.genre);
					break;
				case TagLib::TagType::ID3v2:
					cout << "\tpreferedType: ID3v2" << endl;
					f.U_ID3v2=f.U_Tag;
					ID3v2Tag=f.ID3v2Tag(true);
					ID3v2Tag->setTitle(f.U_Tag.title);
					ID3v2Tag->setArtist(f.U_Tag.artist);
					ID3v2Tag->setAlbum(f.U_Tag.album);
					ID3v2Tag->setComment(f.U_Tag.comment);
					ID3v2Tag->setGenre(f.U_Tag.genre);
					break;
				case TagLib::TagType::MP4:
					cout << "\tpreferedType: MP4" << endl;
					f.U_MP4=f.U_Tag;
					MP4Tag=f.MP4Tag(true);
					MP4Tag->setTitle(f.U_Tag.title);
					MP4Tag->setArtist(f.U_Tag.artist);
					MP4Tag->setAlbum(f.U_Tag.album);
					MP4Tag->setComment(f.U_Tag.comment);
					MP4Tag->setGenre(f.U_Tag.genre);
					break;
				case TagLib::TagType::XiphComment:
					cout << "\tpreferedType: XiphComment" << endl;
					f.U_Xiph=f.U_Tag;
					XiphComment=f.XiphComment(true);
					XiphComment->setTitle(f.U_Tag.title);
					XiphComment->setArtist(f.U_Tag.artist);
					XiphComment->setAlbum(f.U_Tag.album);
					XiphComment->setComment(f.U_Tag.comment);
					XiphComment->setGenre(f.U_Tag.genre);
					break;
				default:
					cerr << "problematic preferedType" << endl;
					return 0;
			}
			printUniTag(f.U_Tag);
		}
		if(testarg==0){
			f.save();
		}
	}else{
		return 0;
	}
	return 1;
}

void usage(){
	cerr << "Usage: bsdtagconv from-conversion[;from-conversion...] [-i inter-conversion] [-r to-conversion] [options..] files" << endl;
	cerr << "Options:" << endl;
	cerr << "\t--notest: Write files" << endl;
	cerr << "\t--noskip: Use conversion results with failure" << endl;
	cerr << "\t--auto: Merge all tags with selectng the best data and write into prefered tag" << endl;
	cerr << "\t--strip: (require --auto) Remove all tags before writing prefered tag" << endl;
	cerr << "\t--each-conv: Don't assume all fields use the same encoding" << endl;
	cerr << "\t--force-decode-all: Decode tag(s) as ID3v1" << endl;
	cerr << "\t--force-decode-ape:" << endl;
	cerr << "\t--force-decode-asf:" << endl;
	cerr << "\t--force-decode-id3v2:" << endl;
	cerr << "\t--force-decode-mp4:" << endl;
	cerr << "\t--force-decode-xiph:" << endl;
	cerr << "\t-i: inter-convertion" << endl;
	cerr << "\t-r: to-conversion for rendering ID3v1" << endl;
}

int main(int argc, char *argv[]){
	int i,argb,intern;
	char *c, *t,*convarg, *arg;

	inter=NULL;
	render=NULL;

	autoarg=0;
	strip=0;
	eachconv=0;
	testarg=1;
	skiparg=1;
	force_decode_all=0;
	force_decode_ape=0;
	force_decode_asf=0;
	force_decode_id3v2=0;
	force_decode_mp4=0;
	force_decode_xiph=0;

	//check
	if(argc<3){
		usage();
		exit(1);
	}

	//initialize
	score=bsdconv_create("utf-8,ascii:normal_score:utf-8,ascii");
	if(score==NULL){
		cerr << bsdconv_error() << endl;
		exit(1);
	}
	convn=1;
	convarg=strdup(argv[1]);
	for(c=convarg;*c;++c)
		if(*c==';')
			++convn;
	convs=(struct bsdconv_instance **)malloc(sizeof(struct bsdconv_instance *)*convn);
	scores=(int *)malloc(sizeof(int)*convn);
	c=convarg;
	for(i=0;i<convn;++i){
		t=strsep(&c, ";");
		if(NULL==(convs[i]=bsdconv_create("ascii:utf-8,ascii"))){
			//exception
			cerr << bsdconv_error() << endl;
			for(i-=1;i>=0;--i){
				bsdconv_destroy(convs[i]);
			}
			free(convs);
			free(convarg);
			cerr << bsdconv_error() << endl;
			exit(1);
		}
		if(bsdconv_replace_phase(convs[i], t, FROM, 0)<0){
			cerr << bsdconv_error() << endl;
			exit(1);
		}
		bsdconv_insert_phase(convs[i], "NORMAL_SCORE", INTER, 1);
		bsdconv_insert_phase(convs[i], "RAW,ASCII", TO, 0);
		bsdconv_insert_phase(convs[i], "UTF-8,ASCII", FROM, 0);
	}
	free(convarg);

	for(argb=2;argb<argc;++argb){
		if(strcmp(argv[argb],"--notest")==0){
			testarg=0;
		}else if(strcmp(argv[argb],"--noskip")==0){
			skiparg=0;
		}else if(strcmp(argv[argb],"--auto")==0){
			autoarg=1;
		}else if(strcmp(argv[argb],"--strip")==0){
			strip=1;
		}else if(strcmp(argv[argb],"--each-conv")==0){
			eachconv=1;
		}else if(strcmp(argv[argb],"--force-decode-all")==0){
			force_decode_all=1;
			force_decode_ape=1;
			force_decode_asf=1;
			force_decode_id3v2=1;
			force_decode_mp4=1;
			force_decode_xiph=1;
		}else if(strcmp(argv[argb],"--force-decode-ape")==0){
			force_decode_ape=1;
		}else if(strcmp(argv[argb],"--force-decode-asf")==0){
			force_decode_asf=1;
		}else if(strcmp(argv[argb],"--force-decode-id3v2")==0){
			force_decode_id3v2=1;
		}else if(strcmp(argv[argb],"--force-decode-mp4")==0){
			force_decode_mp4=1;
		}else if(strcmp(argv[argb],"--force-decode-xiph")==0){
			force_decode_xiph=1;
		}else if(strcmp(argv[argb],"-h")==0 || strcmp(argv[argb],"--help")==0){
			usage();
			exit(0);
		}else if(strcmp(argv[argb],"-r")==0){
			if(argb+1<argc){
				argb+=1;
				if(NULL==(render=bsdconv_create("utf-8,ascii:utf-8,ascii"))){
					cerr << bsdconv_error() << endl;
					exit(1);
				}
				arg=strdup(argv[argb]);
				intern=1;
				for(c=arg;*c;++c)
					if(*c==':')
						++intern;
				c=arg;
				for(i=0;i<intern;++i){
					t=strsep(&c, ":");
					if(bsdconv_replace_phase(render, t, TO, -1)<0){
						cerr << bsdconv_error() << endl;
						exit(1);
					}
				}
				free(arg);
			}else{
				cerr << "Missing argument for -r" << endl;
				exit(1);
			}
		}else if(strcmp(argv[argb],"-i")==0){
			if(argb+1<argc){
				argb+=1;
				if(NULL==(inter=bsdconv_create("utf-8,ascii:utf-8,ascii"))){
					for(i=0;i<convn;++i){
						bsdconv_destroy(convs[i]);
					}
					free(convs);
					cerr << bsdconv_error() << endl;
					exit(1);
				}
				arg=strdup(argv[argb]);
				intern=1;
				for(c=arg;*c;++c)
					if(*c==':')
						++intern;
				c=arg;
				for(i=0;i<intern;++i){
					t=strsep(&c, ":");
					if(bsdconv_insert_phase(inter, t, INTER, i+1)<0){
						cerr << bsdconv_error() << endl;
						exit(1);
					}
				}
				free(arg);
			}else{
				cerr << "Missing argument for -i" << endl;
				exit(1);
			}
		}else if(strcmp(argv[argb],"--")==0){
			argb+=1;
			break;
		}else{
			break;
		}
	}

	if(!autoarg && strip){
		cerr << "Without --auto, --strip is disabled." << endl;
		exit(1);
	}

	TagLib::ID3v1::Tag::setStringHandler( new ID3v1StringHandler() );

	//proceed
	for(i=argb;i<argc;++i){
		proc(argv[i]);
	}

	//cleanup
	for(i=0;i<convn;++i){
		bsdconv_destroy(convs[i]);
	}
	free(convs);
	free(scores);
	bsdconv_destroy(score);
	if(inter)
		bsdconv_destroy(inter);

	if(testarg)
		cerr << endl << "Use --notest to actually write the files" << endl;

	return 0;
}
