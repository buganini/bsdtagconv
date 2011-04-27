#include <iostream>
#include <cstring>
#include <string>
#include <string.h>
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

int convn,testarg,skiparg;
struct bsdconv_instance **convs;

const char * conv(TagLib::Tag *tag, int field){
	int i;
	const char *s=NULL;
	struct bsdconv_instance *ins=NULL;
	switch(field){
		case TITLE:
			s=tag->title().to8Bit(true).c_str();
			break;
		case ARTIST:
			s=tag->artist().to8Bit(true).c_str();
			break;
		case ALBUM:
			s=tag->album().to8Bit(true).c_str();
			break;
		case COMMENT:
			s=tag->comment().to8Bit(true).c_str();
			break;
		case GENRE:
			s=tag->genre().to8Bit(true).c_str();
			break;
	}
	for(i=0;i<convn;++i){
		ins=convs[i];
		bsdconv_init(ins);
		ins->output_mode=BSDCONV_HOLD;
		ins->input.data=(void *)s;
		ins->input.len=strlen(s);
		ins->input.flags=0;
		ins->flush=1;
		bsdconv(ins);
		if(ins->ierr + ins->oerr==0){
			ins->output_mode=BSDCONV_AUTOMALLOC;
			bsdconv(ins);
			break;
		}
	}
	TagLib::String res((const char *)ins->output.data, TagLib::String::UTF8);
	if(i<convn || skiparg==0){
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
	return res.to8Bit(true).c_str();
}

int proc(char *file){
	TagLib::FileRef f(file);
	if(!f.isNull() && f.tag()) {
		TagLib::Tag *tag = f.tag();
		cout << "\ttitle   - \"" << conv(tag, TITLE)   << "\"" << endl;
		cout << "\tartist  - \"" << conv(tag, ARTIST)  << "\"" << endl;
		cout << "\talbum   - \"" << conv(tag, ALBUM)   << "\"" << endl;
		cout << "\tyear    - \"" << tag->year()    << "\"" << endl;
		cout << "\tcomment - \"" << conv(tag, COMMENT) << "\"" << endl;
		cout << "\ttrack   - \"" << tag->track()   << "\"" << endl;
		cout << "\tgenre   - \"" << conv(tag, GENRE)   << "\"" << endl;
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
		fprintf(stderr, "Usage: %s conversion[;conversion...] files\n", argv[0]);
		exit(1);
	}

	//initialize
	convn=1;
	convarg=strdup(argv[1]);
	for(c=convarg;*c;++c)
		if(*c==';')
			++convn;
	convs=(struct bsdconv_instance **)malloc(sizeof(struct bsdconv_instance *));
	c=convarg;
	for(i=0;i<convn;++i){
		t=strsep(&c, ";");
		if(NULL==(convs[i]=bsdconv_create(t))){
			//exception
			for(i-=1;i>=0;--i){
				bsdconv_destroy(convs[i]);
			}
			free(convs);
			free(convarg);
			exit(1);
		}
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

	if(testarg)
		fprintf(stderr, "\nUse --notest to actually write the files\n");

	return 0;
}
