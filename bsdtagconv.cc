#include <iostream>
#include <string.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/tstring.h>
#include <bsdconv.h>

using namespace std;

int convn;
struct bsdconv_instance **convs;

char * conv(const char *s){
	int i;
	struct bsdconv_instance *ins;
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
			return (char *)ins->output.data;
		}
	}
	return "";
}

int proc(char *file){
	TagLib::FileRef f(file);
	if(!f.isNull() && f.tag()) {
		TagLib::Tag *tag = f.tag();
		cout << "\ttitle   - \"" << conv(tag->title().toCString(true))   << "\"" << endl;
		cout << "\tartist  - \"" << conv(tag->artist().toCString(true))  << "\"" << endl;
		cout << "\talbum   - \"" << conv(tag->album().toCString(true))   << "\"" << endl;
		cout << "\tyear    - \"" << tag->year()    << "\"" << endl;
		cout << "\tcomment - \"" << tag->comment() << "\"" << endl;
		cout << "\ttrack   - \"" << tag->track()   << "\"" << endl;
		cout << "\tgenre   - \"" << tag->genre()   << "\"" << endl;
	}
	return 1;
}

int main(int argc, char *argv[]){
	int i;
	char *c, *t,*convarg=strdup(argv[1]);

	//check
	if(argc<3){
		fprintf(stderr, "Usage: %s conversion[;conversion...] files\n", argv[0]);
		exit(1);
	}

	//initialize
	convn=1;
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

	//proceed
	for(i=2;i<argc;++i){
		cout << argv[i] << endl;
		proc(argv[i]);
	}

	//cleanup
	for(i=0;i<convn;++i){
		bsdconv_destroy(convs[i]);
	}
	free(convs);

	return 0;
}
