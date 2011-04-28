#include <iostream>
#include <cstring>
#include <string>
#include <string.h>
#include <taglib/tbytevector.h>
#include <taglib/id3v1tag.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
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

int convn,testarg,skiparg,skip;
struct bsdconv_instance **convs;
int *score;

class ID3v1StringHandler : public TagLib::ID3v1::StringHandler
{
	virtual TagLib::String parse( const TagLib::ByteVector &data ) const
	{
		int i,max;
		struct bsdconv_instance *ins=NULL;
		for(i=0;i<convn;++i){
			ins=convs[i];
			bsdconv_init(ins);
			ins->output_mode=BSDCONV_HOLD;
			ins->input.data=(void *)data.data();
			ins->input.len=data.size();
			ins->input.flags=0;
			ins->flush=1;
			bsdconv(ins);
			score[i]=ins->score + ins->ierr*(-3) + ins->oerr*(-2);
		}
		max=0;
		for(i=0;i<convn;++i){
			if(score[i]>score[max]){
				max=i;
			}
		}
		ins=convs[max];
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
	virtual TagLib::ByteVector render( const TagLib::String &s ) const
	{
		return TagLib::ByteVector("", 0);
	}
};

TagLib::String conv(TagLib::Tag *tag, int field){
	TagLib::String res;
	switch(field){
		case TITLE:
			res=TagLib::String(tag->title().to8Bit(true).c_str(), TagLib::String::UTF8);
			break;
		case ARTIST:
			res=TagLib::String(tag->artist().to8Bit(true).c_str(), TagLib::String::UTF8);
			break;
		case ALBUM:
			res=TagLib::String(tag->album().to8Bit(true).c_str(), TagLib::String::UTF8);
			break;
		case COMMENT:
			res=TagLib::String(tag->comment().to8Bit(true).c_str(), TagLib::String::UTF8);
			break;
		case GENRE:
			res=TagLib::String(tag->genre().to8Bit(true).c_str(), TagLib::String::UTF8);
			break;
		default:
			return "";
	}
	if(skip==0 || skiparg==0){
		if(testarg==0){
			switch(field){
				case TITLE:
					tag->setTitle(res);
					break;
				case ARTIST:
					tag->setArtist(res);
					break;
				case ALBUM:
					tag->setAlbum(res);
					break;
				case COMMENT:
					tag->setComment(res);
					break;
				case GENRE:
					tag->setGenre(res);
					break;
			}
		}
	}
	return res;
}

int proc(char *file){
	TagLib::FileRef f(file);
	if(!f.isNull() && f.tag()) {
		TagLib::Tag *tag = f.tag();
		cout << "\ttitle   - \"" << conv(tag, TITLE).to8Bit(true).c_str()   << "\"" << endl;
		cout << "\tartist  - \"" << conv(tag, ARTIST).to8Bit(true).c_str()  << "\"" << endl;
		cout << "\talbum   - \"" << conv(tag, ALBUM).to8Bit(true).c_str()   << "\"" << endl;
		cout << "\tyear    - \"" << tag->year()    << "\"" << endl;
		cout << "\tcomment - \"" << conv(tag, COMMENT).to8Bit(true).c_str() << "\"" << endl;
		cout << "\ttrack   - \"" << tag->track()   << "\"" << endl;
		cout << "\tgenre   - \"" << conv(tag, GENRE).to8Bit(true).c_str()   << "\"" << endl;
	}else{
		return 0;
	}
	if(testarg==0){
		f.save();
	}
	return 1;
}

int main(int argc, char *argv[]){
	int i,argb;
	char *c, *t,*convarg;

	testarg=1;
	skiparg=1;

	//check
	if(argc<3){
		cerr << "Usage: bsdtagconv from_conversion[;from_conversion...] [-i inter_conversion] files" << endl;
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
			exit(1);
		}
//		bsdconv_insert_codec(convs[i], (char *)"NORMAL_SCORE", bsdconv_insert_phase(convs[i], INTER, 1), 0);
	}
	free(convarg);

	for(argb=2;argb<argc;++argb){
		if(strcmp(argv[argb],"--notest")==0){
			testarg=0;
		}else if(strcmp(argv[argb],"--noskip")==0){
			skiparg=0;
		}else if(strcmp(argv[argb],"--")==0){
			argb+=1;
			break;
		}else{
			break;
		}
	}

	TagLib::ID3v1::Tag::setStringHandler( new ID3v1StringHandler() );
	//proceed
	for(i=argb;i<argc;++i){
		cout << argv[i] << endl;
		proc(argv[i]);
	}

	//cleanup
	for(i=0;i<convn;++i){
		bsdconv_destroy(convs[i]);
	}
	free(convs);
	free(score);

	if(testarg)
		cerr << endl << "Use --notest to actually write the files" << endl;

	return 0;
}
