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

int convn,testarg,skiparg,skip,force_decode_id3v2;
struct bsdconv_instance **convs;
struct bsdconv_instance *inter;
int *score;
int bestCodec;

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

TagLib::String conv(TagLib::String res, const char *field){
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
	cout << "\t\t" << field << ": " << ret.to8Bit(true) << endl;
	return ret;
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
			APETag->setTitle(conv(APETag->title(),"Title"));
			APETag->setArtist(conv(APETag->artist(),"Artist"));
			APETag->setAlbum(conv(APETag->album(),"Album"));
			APETag->setComment(conv(APETag->comment(),"Comment"));
			APETag->setGenre(conv(APETag->genre(),"Genre"));
		}
		if(ASFTag==NULL && f.hasASFTag()){
			cout << "\tASF Tag:" << endl;
			ASFTag=f.ASFTag(false);
			ASFTag->setTitle(conv(ASFTag->title(),"Title"));
			ASFTag->setArtist(conv(ASFTag->artist(),"Artist"));
			ASFTag->setAlbum(conv(ASFTag->album(),"Album"));
			ASFTag->setComment(conv(ASFTag->comment(),"Comment"));
			ASFTag->setGenre(conv(ASFTag->genre(),"Genre"));
			ASFTag->setRating(conv(ASFTag->rating(),"Rating"));
			ASFTag->setCopyright(conv(ASFTag->copyright(),"Copyright"));
		}
		if(ID3v1Tag==NULL && f.hasID3v1Tag()){
			cout << "\tID3v1 Tag:" << endl;
			ID3v1Tag=f.ID3v1Tag(false);
			autoconv_init();
			autoconv_test(ID3v1Tag->title());
			autoconv_test(ID3v1Tag->artist());
			autoconv_test(ID3v1Tag->album());
			autoconv_test(ID3v1Tag->comment());
			autoconv_test(ID3v1Tag->genre());
			ID3v1Tag->setTitle(conv(autoconv(ID3v1Tag->title()),"Title"));
			ID3v1Tag->setArtist(conv(autoconv(ID3v1Tag->artist()),"Artist"));
			ID3v1Tag->setAlbum(conv(autoconv(ID3v1Tag->album()),"Album"));
			ID3v1Tag->setComment(conv(autoconv(ID3v1Tag->comment()),"Comment"));
			ID3v1Tag->setGenre(conv(autoconv(ID3v1Tag->genre()),"Genre"));
		}
		if(ID3v2Tag==NULL && f.hasID3v2Tag()){
			cout << "\tID3v2 Tag:" << endl;
			ID3v2Tag=f.ID3v2Tag(false);
			if(force_decode_id3v2){
				autoconv_init();
				autoconv_test(ID3v2Tag->title());
				autoconv_test(ID3v2Tag->artist());
				autoconv_test(ID3v2Tag->album());
				autoconv_test(ID3v2Tag->comment());
				autoconv_test(ID3v2Tag->genre());
				ID3v2Tag->setTitle(conv(autoconv(ID3v2Tag->title()),"Title"));
				ID3v2Tag->setArtist(conv(autoconv(ID3v2Tag->artist()),"Artist"));
				ID3v2Tag->setAlbum(conv(autoconv(ID3v2Tag->album()),"Album"));
				ID3v2Tag->setComment(conv(autoconv(ID3v2Tag->comment()),"Comment"));
				ID3v2Tag->setGenre(conv(autoconv(ID3v2Tag->genre()),"Genre"));

			}else{
				ID3v2Tag->setTitle(conv(ID3v2Tag->title(),"Title"));
				ID3v2Tag->setArtist(conv(ID3v2Tag->artist(),"Artist"));
				ID3v2Tag->setAlbum(conv(ID3v2Tag->album(),"Album"));
				ID3v2Tag->setComment(conv(ID3v2Tag->comment(),"Comment"));
				ID3v2Tag->setGenre(conv(ID3v2Tag->genre(),"Genre"));
			}
		}
		if(MP4Tag==NULL && f.hasMP4Tag()){
			cout << "\tMP4 Tag:" << endl;
			MP4Tag=f.MP4Tag(false);
			MP4Tag->setTitle(conv(MP4Tag->title(),"Title"));
			MP4Tag->setArtist(conv(MP4Tag->artist(),"Artist"));
			MP4Tag->setAlbum(conv(MP4Tag->album(),"Album"));
			MP4Tag->setComment(conv(MP4Tag->comment(),"Comment"));
			MP4Tag->setGenre(conv(MP4Tag->genre(),"Genre"));
		}
		if(XiphComment==NULL && f.hasXiphComment()){
			cout << "\tXiphComment Tag:" << endl;
			XiphComment=f.XiphComment(false);
			XiphComment->setTitle(conv(XiphComment->title(),"Title"));
			XiphComment->setArtist(conv(XiphComment->artist(),"Artist"));
			XiphComment->setAlbum(conv(XiphComment->album(),"Album"));
			XiphComment->setComment(conv(XiphComment->comment(),"Comment"));
			XiphComment->setGenre(conv(XiphComment->genre(),"Genre"));
		}
		if(f.hasTag()){
			cout << "\tTag:" << endl;
			Tag=f.tag();
			Tag->setTitle(conv(Tag->title(),"Title"));
			Tag->setArtist(conv(Tag->artist(),"Artist"));
			Tag->setAlbum(conv(Tag->album(),"Album"));
			Tag->setComment(conv(Tag->comment(),"Comment"));
			Tag->setGenre(conv(Tag->genre(),"Genre"));
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
	force_decode_id3v2=0;

	//check
	if(argc<3){
		cerr << "Usage: bsdtagconv from_conversion[;from_conversion...] [-i inter_conversion] [options..] files" << endl;
		cerr << "Options:" << endl;
		cerr << "\t--force-decode-id3v2: decode ID3v2 as ID3v1" << endl;
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
		}else if(strcmp(argv[argb],"--force-decode-id3v2")==0){
			force_decode_id3v2=1;
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
