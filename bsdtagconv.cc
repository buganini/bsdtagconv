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
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/apetag.h>
#include <taglib/asftag.h>
#include <taglib/id3v1tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4tag.h>
#include <taglib/xiphcomment.h>
#include <taglib/tstring.h>
#include <bsdconv.h>

enum field{
	TITLE,
	ARTIST,
	ALBUM,
	COMMENT,
	GENRE
};

using namespace std;

int convn;
int testarg,skiparg,skip;
int force_decode_all;
int force_decode_ape;
int force_decode_asf;
int force_decode_id3v2;
int force_decode_mp4;
int force_decode_xiph;

struct bsdconv_instance **convs;
struct bsdconv_instance *inter;
int *score;
int bestCodec;

class UniTag {
	public:
	TagLib::String title;
	TagLib::String artist;
	TagLib::String album;
	TagLib::String comment;
	TagLib::String genre;
	TagLib::String rating;
	TagLib::String copyright;
	UniTag(){
		title=TagLib::String::null;
		artist=TagLib::String::null;
		album=TagLib::String::null;
		comment=TagLib::String::null;
		genre=TagLib::String::null;
		rating=TagLib::String::null;
		copyright=TagLib::String::null;
	}
};

void autoconv_init(){
	int i;
	for(i=0;i<convn;++i){
		score[i]=0;
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
		score[i]+=ins->score + ins->ierr*(-3) + ins->oerr*(-2);
	}
	max=0;
	for(i=0;i<convn;++i){
		if(score[i]>score[max]){
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

void autoConv(UniTag &U){
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

void Conv(UniTag &U){
	if(!U.title.isNull()){
		U.title=conv(U.title);
		cout << "\t\tTitle: " << U.title.to8Bit(true) << endl;
	}
	if(!U.artist.isNull()){
		 U.artist=conv(U.artist);
		cout << "\t\tArtist: " << U.artist.to8Bit(true) << endl;
	}
	if(!U.album.isNull()){
		 U.album=conv(U.album);
		cout << "\t\tAlbum: " << U.album.to8Bit(true) << endl;
	}
	if(!U.comment.isNull()){
		 U.comment=conv(U.comment);
		cout << "\t\tComment: " << U.comment.to8Bit(true) << endl;
	}
	if(!U.genre.isNull()){
		 U.genre=conv(U.genre);
		cout << "\t\tGenre: " << U.genre.to8Bit(true) << endl;
	}
	if(!U.rating.isNull()){
		 U.genre=conv(U.rating);
		cout << "\t\tRating: " << U.rating.to8Bit(true) << endl;
	}
	if(!U.copyright.isNull()){
		 U.genre=conv(U.copyright);
		cout << "\t\tCopyright: " << U.copyright.to8Bit(true) << endl;
	}
}

int proc(char *file){
	UniTag U;
	TagLib::APE::Tag * APETag=NULL;
	TagLib::ASF::Tag * ASFTag=NULL;
	TagLib::ID3v1::Tag * ID3v1Tag=NULL;
	TagLib::ID3v2::Tag * ID3v2Tag=NULL;
	TagLib::MP4::Tag * MP4Tag=NULL;
	TagLib::Ogg::XiphComment * XiphComment=NULL;
	TagLib::Tag * Tag=NULL;
	TagLib::FileRef f(file);
	if(!f.isNull()){
		cout << file << "  ";
		switch(f.preferedType()){
			case TagLib::Type::None:
				cout << "preferedType: None" << endl;
				break;
			case TagLib::Type::APE:
				cout << "preferedType: APE" << endl;
				if(!f.hasAPETag()){
					APETag=f.APETag(true);
				}
				break;
			case TagLib::Type::ASF:
				cout << "preferedType: ASF" << endl;
				if(!f.hasASFTag()){
					ASFTag=f.ASFTag(true);
				}
				break;
			case TagLib::Type::ID3v1:
				cout << "preferedType: ID3v1" << endl;
				if(!f.hasID3v1Tag()){
					ID3v1Tag=f.ID3v1Tag(true);
				}
				break;
			case TagLib::Type::ID3v2:
				cout << "preferedType: ID3v2" << endl;
				if(!f.hasID3v2Tag()){
					ID3v2Tag=f.ID3v2Tag(true);
				}
				break;
			case TagLib::Type::MP4:
				cout << "preferedType: MP4" << endl;
				if(!f.hasMP4Tag()){
					MP4Tag=f.MP4Tag(true);
				}
				break;
			case TagLib::Type::XiphComment:
				cout << "preferedType: XiphComment" << endl;
				if(!f.hasXiphComment()){
					XiphComment=f.XiphComment(true);
				}
				break;
			default:
				cerr << "problematic preferedType" << endl;
				exit(1);
				break;
		}
		if(APETag==NULL && f.hasAPETag()){
			cout << "\tAPE Tag:" << endl;
			APETag=f.APETag(false);
			U=UniTag::UniTag();
			U.title=APETag->title();
			U.artist=APETag->artist();
			U.album=APETag->album();
			U.comment=APETag->comment();
			U.genre=APETag->genre();
			if(force_decode_ape)
				autoConv(U);
			APETag->setTitle(U.title);
			APETag->setArtist(U.artist);
			APETag->setAlbum(U.album);
			APETag->setComment(U.comment);
			APETag->setGenre(U.genre);
		}
		if(ASFTag==NULL && f.hasASFTag()){
			cout << "\tASF Tag:" << endl;
			ASFTag=f.ASFTag(false);
			U=UniTag::UniTag();
			U.title=ASFTag->title();
			U.artist=ASFTag->artist();
			U.album=ASFTag->album();
			U.comment=ASFTag->comment();
			U.genre=ASFTag->genre();
			U.genre=ASFTag->rating();
			U.genre=ASFTag->copyright();
			if(force_decode_asf)
				autoConv(U);
			ASFTag->setTitle(U.title);
			ASFTag->setArtist(U.artist);
			ASFTag->setAlbum(U.album);
			ASFTag->setComment(U.comment);
			ASFTag->setGenre(U.genre);
			ASFTag->setRating(U.rating);
			ASFTag->setCopyright(U.copyright);
		}
		if(ID3v1Tag==NULL && f.hasID3v1Tag()){
			cout << "\tID3v1 Tag:" << endl;
			ID3v1Tag=f.ID3v1Tag(false);
			U=UniTag::UniTag();
			U.title=ID3v1Tag->title();
			U.artist=ID3v1Tag->artist();
			U.album=ID3v1Tag->album();
			U.comment=ID3v1Tag->comment();
			U.genre=ID3v1Tag->genre();
			autoConv(U);
			Conv(U);
			ID3v1Tag->setTitle(U.title);
			ID3v1Tag->setArtist(U.artist);
			ID3v1Tag->setAlbum(U.album);
			ID3v1Tag->setComment(U.comment);
			ID3v1Tag->setGenre(U.genre);
		}
		if(ID3v2Tag==NULL && f.hasID3v2Tag()){
			cout << "\tID3v2 Tag:" << endl;
			ID3v2Tag=f.ID3v2Tag(false);
			U=UniTag::UniTag();
			U.title=ID3v2Tag->title();
			U.artist=ID3v2Tag->artist();
			U.album=ID3v2Tag->album();
			U.comment=ID3v2Tag->comment();
			U.genre=ID3v2Tag->genre();
			if(force_decode_id3v2)
				autoConv(U);
			Conv(U);
			ID3v2Tag->setTitle(U.title);
			ID3v2Tag->setArtist(U.artist);
			ID3v2Tag->setAlbum(U.album);
			ID3v2Tag->setComment(U.comment);
			ID3v2Tag->setGenre(U.genre);
		}
		if(MP4Tag==NULL && f.hasMP4Tag()){
			cout << "\tMP4 Tag:" << endl;
			MP4Tag=f.MP4Tag(false);
			U=UniTag::UniTag();
			U.title=MP4Tag->title();
			U.artist=MP4Tag->artist();
			U.album=MP4Tag->album();
			U.comment=MP4Tag->comment();
			U.genre=MP4Tag->genre();
			if(force_decode_mp4)
				autoConv(U);
			Conv(U);
			MP4Tag->setTitle(U.title);
			MP4Tag->setArtist(U.artist);
			MP4Tag->setAlbum(U.album);
			MP4Tag->setComment(U.comment);
			MP4Tag->setGenre(U.genre);
		}
		if(XiphComment==NULL && f.hasXiphComment()){
			cout << "\tXiphComment Tag:" << endl;
			XiphComment=f.XiphComment(false);
			U=UniTag::UniTag();
			U.title=XiphComment->title();
			U.artist=XiphComment->artist();
			U.album=XiphComment->album();
			U.comment=XiphComment->comment();
			U.genre=XiphComment->genre();
			if(force_decode_xiph)
				autoConv(U);
			Conv(U);
			XiphComment->setTitle(U.title);
			XiphComment->setArtist(U.artist);
			XiphComment->setAlbum(U.album);
			XiphComment->setComment(U.comment);
			XiphComment->setGenre(U.genre);
		}
		if(f.hasTag()){
			cout << "\tTag:" << endl;
			Tag=f.tag();
			U=UniTag::UniTag();
			U.title=Tag->title();
			U.artist=Tag->artist();
			U.album=Tag->album();
			U.comment=Tag->comment();
			U.genre=Tag->genre();
			if(force_decode_all)
				autoConv(U);
			Conv(U);
			Tag->setTitle(U.title);
			Tag->setArtist(U.artist);
			Tag->setAlbum(U.album);
			Tag->setComment(U.comment);
			Tag->setGenre(U.genre);
		}

	}else{
		return 0;
	}
	if(testarg==0){
		f.save();
	}
	return 1;
}

int main(int argc, char *argv[]){
	int i,argb,p;
	char *c, *t,*convarg;

	inter=NULL;

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
		cerr << "Usage: bsdtagconv from_conversion[;from_conversion...] [-i inter_conversion] [options..] files" << endl;
		cerr << "Options:" << endl;
		cerr << "\t--force-decode-all: Decode tag(s) as ID3v1" << endl;
		cerr << "\t--force-decode-ape:" << endl;
		cerr << "\t--force-decode-asf:" << endl;
		cerr << "\t--force-decode-id3v2:" << endl;
		cerr << "\t--force-decode-mp4:" << endl;
		cerr << "\t--force-decode-xiph:" << endl;
		exit(1);
	}

	//initialize
	convn=1;
	convarg=strdup(argv[1]);
	for(c=convarg;*c;++c)
		if(*c==';')
			++convn;
	convs=(struct bsdconv_instance **)malloc(sizeof(struct bsdconv_instance *)*convn);
	score=(int *)malloc(sizeof(score)*convn);
	c=convarg;
	for(i=0;i<convn;++i){
		t=strsep(&c, ";");
		if(NULL==(convs[i]=bsdconv_create(t))){
			//exception
			cerr << bsdconv_error() << endl;
			for(i-=1;i>=0;--i){
				bsdconv_destroy(convs[i]);
			}
			free(convs);
			free(convarg);
			cerr << "Failed create conversion instance: " << bsdconv_error() << endl;
			exit(1);
		}
		p=bsdconv_insert_phase(convs[i], INTER, 1);
		bsdconv_insert_codec(convs[i], (char *)"NORMAL_SCORE", p, 0);
		p=bsdconv_insert_phase(convs[i], TO, 0);
		bsdconv_insert_codec(convs[i], (char *)"ASCII", p, 0);
		bsdconv_insert_codec(convs[i], (char *)"RAW", p, 0);
		p=bsdconv_insert_phase(convs[i], FROM, 0);
		bsdconv_insert_codec(convs[i], (char *)"ASCII", p, 0);
		bsdconv_insert_codec(convs[i], (char *)"UTF-8", p, 0);
	}
	free(convarg);

	for(argb=2;argb<argc;++argb){
		if(strcmp(argv[argb],"--notest")==0){
			testarg=0;
		}else if(strcmp(argv[argb],"--noskip")==0){
			skiparg=0;
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
		}else if(strcmp(argv[argb],"-i")==0){
			if(argb+1<argc){
				argb+=1;
				if(NULL==(inter=bsdconv_create(argv[argb]))){
					for(i=0;i<convn;++i){
						bsdconv_destroy(convs[i]);
					}
					free(convs);
					cerr << "Failed create inter conversion instance: " << bsdconv_error() << endl;
					exit(1);
				}
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

	//proceed
	for(i=argb;i<argc;++i){
		proc(argv[i]);
	}

	//cleanup
	for(i=0;i<convn;++i){
		bsdconv_destroy(convs[i]);
	}
	free(convs);
	free(score);
	if(inter)
		bsdconv_destroy(inter);

	if(testarg)
		cerr << endl << "Use --notest to actually write the files" << endl;

	return 0;
}
