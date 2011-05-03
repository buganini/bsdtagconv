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
int *scores;
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

void autoConv(UniTag &U){
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

void Conv(UniTag &U){
	if(!U.title.isNull()) U.title=conv(U.title);
	if(!U.artist.isNull()) U.artist=conv(U.artist);
	if(!U.album.isNull()) U.album=conv(U.album);
	if(!U.comment.isNull()) U.comment=conv(U.comment);
	if(!U.genre.isNull()) U.genre=conv(U.genre);
	if(!U.rating.isNull()) U.genre=conv(U.rating);
	if(!U.copyright.isNull()) U.genre=conv(U.copyright);
}

void printUniTag(UniTag &U){
	if(!U.title.isNull()) cout << "\t\tTitle: " << U.title.to8Bit(true) << endl;
	if(!U.artist.isNull()) cout << "\t\tArtist: " << U.artist.to8Bit(true) << endl;
	if(!U.album.isNull()) cout << "\t\tAlbum: " << U.album.to8Bit(true) << endl;
	if(!U.comment.isNull()) cout << "\t\tComment: " << U.comment.to8Bit(true) << endl;
	if(!U.genre.isNull()) cout << "\t\tGenre: " << U.genre.to8Bit(true) << endl;
	if(!U.rating.isNull()) cout << "\t\tRating: " << U.rating.to8Bit(true) << endl;
	if(!U.copyright.isNull()) cout << "\t\tCopyright: " << U.copyright.to8Bit(true) << endl;
}

void better(UniTag &A, UniTag &B){
	if(score_eval(A.title) < score_eval(B.title)) A.title=B.title;
	if(score_eval(A.artist) < score_eval(B.artist)) A.artist=B.artist;
	if(score_eval(A.album) < score_eval(B.album)) A.album=B.album;
	if(score_eval(A.comment) < score_eval(B.comment)) A.comment=B.comment;
	if(score_eval(A.genre) < score_eval(B.genre)) A.genre=B.genre;
	if(score_eval(A.rating) < score_eval(B.rating)) A.rating=B.rating;
	if(score_eval(A.copyright) < score_eval(B.copyright)) A.copyright=B.copyright;
}

int proc(char *file){
	UniTag U_APE, U_ASF, U_ID3v1, U_ID3v2, U_MP4, U_Xiph, U_Tag;
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
		if(APETag==NULL && f.APETag()){
			APETag=f.APETag(false);
			U_APE.title=APETag->title();
			U_APE.artist=APETag->artist();
			U_APE.album=APETag->album();
			U_APE.comment=APETag->comment();
			U_APE.genre=APETag->genre();
			if(force_decode_ape)
				autoConv(U_APE);
			Conv(U_APE);
			APETag->setTitle(U_APE.title);
			APETag->setArtist(U_APE.artist);
			APETag->setAlbum(U_APE.album);
			APETag->setComment(U_APE.comment);
			APETag->setGenre(U_APE.genre);
		}
		if(ASFTag==NULL && f.ASFTag()){
			ASFTag=f.ASFTag(false);
			U_ASF.title=ASFTag->title();
			U_ASF.artist=ASFTag->artist();
			U_ASF.album=ASFTag->album();
			U_ASF.comment=ASFTag->comment();
			U_ASF.genre=ASFTag->genre();
			U_ASF.genre=ASFTag->rating();
			U_ASF.genre=ASFTag->copyright();
			if(force_decode_asf)
				autoConv(U_ASF);
			Conv(U_ASF);
			ASFTag->setTitle(U_ASF.title);
			ASFTag->setArtist(U_ASF.artist);
			ASFTag->setAlbum(U_ASF.album);
			ASFTag->setComment(U_ASF.comment);
			ASFTag->setGenre(U_ASF.genre);
			ASFTag->setRating(U_ASF.rating);
			ASFTag->setCopyright(U_ASF.copyright);
		}
		if(ID3v1Tag==NULL && f.ID3v1Tag()){
			ID3v1Tag=f.ID3v1Tag(false);
			U_ID3v1.title=ID3v1Tag->title();
			U_ID3v1.artist=ID3v1Tag->artist();
			U_ID3v1.album=ID3v1Tag->album();
			U_ID3v1.comment=ID3v1Tag->comment();
			U_ID3v1.genre=ID3v1Tag->genre();
			autoConv(U_ID3v1);
			Conv(U_ID3v1);
			ID3v1Tag->setTitle(U_ID3v1.title);
			ID3v1Tag->setArtist(U_ID3v1.artist);
			ID3v1Tag->setAlbum(U_ID3v1.album);
			ID3v1Tag->setComment(U_ID3v1.comment);
			ID3v1Tag->setGenre(U_ID3v1.genre);
		}
		if(ID3v2Tag==NULL && f.ID3v2Tag()){
			ID3v2Tag=f.ID3v2Tag(false);
			U_ID3v2.title=ID3v2Tag->title();
			U_ID3v2.artist=ID3v2Tag->artist();
			U_ID3v2.album=ID3v2Tag->album();
			U_ID3v2.comment=ID3v2Tag->comment();
			U_ID3v2.genre=ID3v2Tag->genre();
			if(force_decode_id3v2)
				autoConv(U_ID3v2);
			Conv(U_ID3v2);
			ID3v2Tag->setTitle(U_ID3v2.title);
			ID3v2Tag->setArtist(U_ID3v2.artist);
			ID3v2Tag->setAlbum(U_ID3v2.album);
			ID3v2Tag->setComment(U_ID3v2.comment);
			ID3v2Tag->setGenre(U_ID3v2.genre);
		}
		if(MP4Tag==NULL && f.MP4Tag()){
			MP4Tag=f.MP4Tag(false);
			U_MP4.title=MP4Tag->title();
			U_MP4.artist=MP4Tag->artist();
			U_MP4.album=MP4Tag->album();
			U_MP4.comment=MP4Tag->comment();
			U_MP4.genre=MP4Tag->genre();
			if(force_decode_mp4)
				autoConv(U_MP4);
			Conv(U_MP4);
			MP4Tag->setTitle(U_MP4.title);
			MP4Tag->setArtist(U_MP4.artist);
			MP4Tag->setAlbum(U_MP4.album);
			MP4Tag->setComment(U_MP4.comment);
			MP4Tag->setGenre(U_MP4.genre);
		}
		if(XiphComment==NULL && f.XiphComment()){
			XiphComment=f.XiphComment(false);
			U_Xiph.title=XiphComment->title();
			U_Xiph.artist=XiphComment->artist();
			U_Xiph.album=XiphComment->album();
			U_Xiph.comment=XiphComment->comment();
			U_Xiph.genre=XiphComment->genre();
			if(force_decode_xiph)
				autoConv(U_Xiph);
			Conv(U_Xiph);
			XiphComment->setTitle(U_Xiph.title);
			XiphComment->setArtist(U_Xiph.artist);
			XiphComment->setAlbum(U_Xiph.album);
			XiphComment->setComment(U_Xiph.comment);
			XiphComment->setGenre(U_Xiph.genre);
		}
		if(f.tag()){
			Tag=f.tag();
			U_Tag.title=Tag->title();
			U_Tag.artist=Tag->artist();
			U_Tag.album=Tag->album();
			U_Tag.comment=Tag->comment();
			U_Tag.genre=Tag->genre();
			if(force_decode_all)
				autoConv(U_Tag);
			Conv(U_Tag);
			Tag->setTitle(U_Tag.title);
			Tag->setArtist(U_Tag.artist);
			Tag->setAlbum(U_Tag.album);
			Tag->setComment(U_Tag.comment);
			Tag->setGenre(U_Tag.genre);
		}
		if(autoarg){
			better(U_Tag, U_APE);
			better(U_Tag, U_ASF);
			better(U_Tag, U_ID3v1);
			better(U_Tag, U_ID3v2);
			better(U_Tag, U_MP4);
			better(U_Tag, U_Xiph);
		}
		if(!strip && f.APETag()){
			cout << "\tAPE Tag:" << endl;
			printUniTag(U_APE);
		}
		if(!strip && f.ASFTag()){
			cout << "\tASF Tag:" << endl;
			printUniTag(U_ASF);		
		}
		if(!strip && f.ID3v1Tag()){
			cout << "\tID3v1 Tag:" << endl;
			printUniTag(U_ID3v1);
		}
		if(!strip && f.ID3v2Tag()){
			cout << "\tID3v2 Tag:" << endl;
			printUniTag(U_ID3v2);
		}
		if(!strip && f.MP4Tag()){
			cout << "\tMP4 Tag:" << endl;
			printUniTag(U_MP4);
		}
		if(!strip && f.XiphComment()){
			cout << "\tXiphComment Tag:" << endl;
			printUniTag(U_Xiph);
		}
		if(!strip && f.tag()){
			cout << "\tTag:" << endl;
			printUniTag(U_Tag);
		}
		if(!testarg && autoarg && strip){
			f.strip(~0);
			f.save();
		}
		if(autoarg){
			switch(f.preferedTag()){
				case TagLib::TagType::None:
					cout << "\tpreferedType: None" << endl;
					break;
				case TagLib::TagType::APE:
					cout << "\tpreferedType: APE" << endl;
					APETag=f.APETag(true);
					APETag->setTitle(U_Tag.title);
					APETag->setArtist(U_Tag.artist);
					APETag->setAlbum(U_Tag.album);
					APETag->setComment(U_Tag.comment);
					APETag->setGenre(U_Tag.genre);
					break;
				case TagLib::TagType::ASF:
					cout << "\tpreferedType: ASF" << endl;
					ASFTag=f.ASFTag(true);
					ASFTag->setTitle(U_Tag.title);
					ASFTag->setArtist(U_Tag.artist);
					ASFTag->setAlbum(U_Tag.album);
					ASFTag->setComment(U_Tag.comment);
					ASFTag->setGenre(U_Tag.genre);
					ASFTag->setRating(U_Tag.rating);
					ASFTag->setCopyright(U_Tag.copyright);
					break;
				case TagLib::TagType::ID3v1:
					cout << "\tpreferedType: ID3v1" << endl;
					ID3v1Tag=f.ID3v1Tag(true);
					ID3v1Tag->setTitle(U_Tag.title);
					ID3v1Tag->setArtist(U_Tag.artist);
					ID3v1Tag->setAlbum(U_Tag.album);
					ID3v1Tag->setComment(U_Tag.comment);
					ID3v1Tag->setGenre(U_Tag.genre);
					break;
				case TagLib::TagType::ID3v2:
					cout << "\tpreferedType: ID3v2" << endl;
					ID3v2Tag=f.ID3v2Tag(true);
					ID3v2Tag->setTitle(U_Tag.title);
					ID3v2Tag->setArtist(U_Tag.artist);
					ID3v2Tag->setAlbum(U_Tag.album);
					ID3v2Tag->setComment(U_Tag.comment);
					ID3v2Tag->setGenre(U_Tag.genre);
					break;
				case TagLib::TagType::MP4:
					cout << "\tpreferedType: MP4" << endl;
					MP4Tag=f.MP4Tag(true);
					MP4Tag->setTitle(U_Tag.title);
					MP4Tag->setArtist(U_Tag.artist);
					MP4Tag->setAlbum(U_Tag.album);
					MP4Tag->setComment(U_Tag.comment);
					MP4Tag->setGenre(U_Tag.genre);
					break;
				case TagLib::TagType::XiphComment:
					cout << "\tpreferedType: XiphComment" << endl;
					XiphComment=f.XiphComment(true);
					XiphComment->setTitle(U_Tag.title);
					XiphComment->setArtist(U_Tag.artist);
					XiphComment->setAlbum(U_Tag.album);
					XiphComment->setComment(U_Tag.comment);
					XiphComment->setGenre(U_Tag.genre);
					break;
				default:
					cerr << "problematic preferedType" << endl;
					return 0;
			}
			printUniTag(U_Tag);
		}
		if(testarg==0){
			f.save();
		}
	}else{
		return 0;
	}
	return 1;
}

int main(int argc, char *argv[]){
	int i,argb;
	char *c, *t,*convarg;

	inter=NULL;

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
		cerr << "Usage: bsdtagconv from_conversion[;from_conversion...] [-i inter_conversion] [options..] files" << endl;
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

	if(!autoarg && strip){
		cerr << "Without --auto, --strip is disabled." << endl;
		exit(1);
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
	free(scores);
	bsdconv_destroy(score);
	if(inter)
		bsdconv_destroy(inter);

	if(testarg)
		cerr << endl << "Use --notest to actually write the files" << endl;

	return 0;
}
