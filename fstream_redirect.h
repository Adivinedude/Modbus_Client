#ifndef FSTREAM_REDIRECT_H
#define FSTREAM_REDIRECT_H
	#include <iostream>
	#include <sstream>
	//#include "stdio.h"
	

	char fstream_redirect_buffer[BUFSIZ+1];

	class fstream_redirect {
	public:
		fstream_redirect(std::stringstream &new_buffer, std::ostream &fstream, FILE* c_handle){
			
			//save the pointers needed later
			_new_buffer = &new_buffer;
			_old		= fstream.rdbuf();
			_fstream	= &fstream;

			//Direct C++ calls to std::cout, std::cin, or std::cerr	to a new_buffer
			fstream.rdbuf(new_buffer.rdbuf());

			//Direct C calls to stdout, stdin, or stderr to a new_buffer. 
			//This is not easy in C, as it is in C++
			// Order of Operations
			// 1) check buffer for change
			// 2) null terminate the new data
			// 3) retrive data
			// 4) rewind file handle to begining
			// 5) GoTo step 1

			//send stream to a void	so it does not print
			#ifdef _WIN32
				_file = freopen("nul", "a", c_handle);
			#else
				_file = freopen("/dev/null", "a", c_handle);
			#endif	
			//set the _file to use our temp buffer
			if( setvbuf( _file, fstream_redirect_buffer, _IOFBF, BUFSIZ) )
				std::cout << "setvbuf failed";
		}

		~fstream_redirect() {
			//C++ restore the stream to the correct output.
			_fstream->rdbuf(_old);
			//C figure out how to restore the stream to the proper output
		}

		void update(){
			fpos_t pos;
			char *p = fstream_redirect_buffer;
			fgetpos(_file, &pos);
			//Poll the C buffer for new data
			if(pos){
				p[pos] = 0;				//null terminate the data
				*_new_buffer << p;		//send data to the new_buffer
				rewind(_file);			//reset the next write to the start of the buffer
			}
		}

	private:
		fstream_redirect() {};
		std::stringstream*	_new_buffer;
		std::streambuf*		_old;
		std::ios*			_fstream;
		FILE*				_file;
	};

#endif