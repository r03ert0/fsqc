char version[]="fsqc, version 1, roberto toro, 24 June 2010";
/*
fsqc:	FreeSurfer Quality Control, save a picture of a freesurfer
		mesh with labels superimposed for a subject.
		
Usage example:
./fsqc -sub freesurfer/subjects/bert/ -out cuiccuic.tif -ori lat -hem lh

on Mac OS X:
gcc -o fsqc fsqc.c -framework Carbon -framework OpenGL -framework GLUT

To compile on Unix:
gcc -o fsqc fsqc.c -lGL -lGLU -lglut

on Windows:
gcc -o fsqc.exe fsqc.c -lopengl32 -lglut32
*/

#define __MacOSX__
//#define __UnixOrWindows__

#ifdef __MacOSX__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#endif

#ifdef __UnixOrWindows__
  #include <GL/gl.h>
  #include <GL/glut.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct
{
	float	x,y,z;
}float3D;
typedef struct
{
	int	a,b,c;
}int3D;

float3D	*p,*an;
int3D	*t;
int		np;
int		nt;
float3D	back;

#define min(a,b) (((a)<(b))?(a):(b))
#define kZoom 2.0

#pragma mark -
int		endianness;
#define kMOTOROLA	1
#define kINTEL		2
void checkEndianness(void)
{
	char	b[]={1,0,0,0};
	int		num=*(int*)b;
	
	if(num==16777216)
		endianness=kMOTOROLA;
	else
		endianness=kINTEL;
}

void swapint(int *n)
{
	char 	*by=(char*)n;
	char	sw[4]={by[3],by[2],by[1],by[0]};
	
	*n=*(int*)sw;
}
void swapfloat(float *n)
{
	char 	*by=(char*)n;
	char	sw[4]={by[3],by[2],by[1],by[0]};
	
	*n=*(float*)sw;
}
void swaptriangles()
{
	int		i;
	for(i=0;i<nt;i++)
	{
		swapint(&t[i].a);
		swapint(&t[i].b);
		swapint(&t[i].c);
	}
}
void swapvertices()
{
	int		i;
	for(i=0;i<np;i++)
	{
		swapfloat(&p[i].x);
		swapfloat(&p[i].y);
		swapfloat(&p[i].z);
	}
}
#pragma mark -
int FreeSurfer_load_mesh(char *path)
{
    FILE	*f;
    int		id,a,b,c,i;
    char	date[256],info[256];

    f=fopen(path,"r");
	
	if(f==NULL)
		return 1;

	// read triangle/quad identifier: 3 bytes
    a=((int)(u_int8_t)fgetc(f))<<16;
    b=((int)(u_int8_t)fgetc(f))<<8;
    c=(u_int8_t)fgetc(f);
    id=a+b+c;
    if(id==16777214)	// triangle mesh
    {
		fgets(date,256,f);
        fgets(info,256,f);
		fread(&np,1,sizeof(int),f); if(endianness==kINTEL) swapint(&np);
		fread(&nt,1,sizeof(int),f);	if(endianness==kINTEL) swapint(&nt);
        // read vertices
        p=(float3D*)calloc(np,sizeof(float3D));
			if(p==NULL) printf("ERROR: Cannot allocate memory for points [FreeSurfer_load_mesh]\n");
			else
			{
		fread((char*)p,np,3*sizeof(float),f);	if(endianness==kINTEL) swapvertices();
        // read triangles
        t=(int3D*)calloc(nt,sizeof(int3D));
			if(t==NULL) printf("ERROR: Cannot allocate memory for triangles [FreeSurfer_load_mesh]\n");
			else
			{
		fread((char*)t,nt,3*sizeof(int),f);		if(endianness==kINTEL) swaptriangles();
			}
			}
    }
	fclose(f);

	for(i=0;i<np;i++)
	{
		p[i].x+=128;
		p[i].y+=128;
		p[i].z+=128;
	}
	
	return 0;
}
int FreeSurfer_load_annotation(char *path)
// endianness aware
{
	FILE	*f;
	int		i,j,n,nlab=35,l;
	char	*tmp;
	int		err=1;
	int		lab[35][3]={{ 25,  5, 25}, { 25,100, 40}, {125,100,160}, {100, 25,  0}, {120, 70, 50},
						{220, 20,100}, {220, 20, 10}, {180,220,140}, {220, 60,220}, {180, 40,120},
						{140, 20,140}, { 20, 30,140}, { 35, 75, 50}, {225,140,140}, {200, 35, 75},
						{160,100, 50}, { 20,220, 60}, { 60,220, 60}, {220,180,140}, { 20,100, 50},
						{220, 60, 20}, {120,100, 60}, {220, 20, 20}, {220,180,220}, { 60, 20,220},
						{160,140,180}, { 80, 20,140}, { 75, 50,125}, { 20,220,160}, { 20,180,140},
						{140,220,220}, { 80,160, 20}, {100,  0,100}, { 70, 70, 70}, {150,150,200}};

    f=fopen(path,"r");
	if(f==NULL)
		return 0;
	
	fread(&n,1,sizeof(int),f);	if(endianness==kINTEL) swapint(&n);
			if(n!=np) printf("Annotation file corrupted. points:%i annotations:%i [FreeSurfer_load_annotation]\n",np,n);
			else
			{
	tmp=calloc(np,2*sizeof(int));
			if(tmp==NULL) printf("Cannot allocate memory for tmp [FreeSurfer_load_annotation]\n");
			else
			{
	fread(tmp,n,2*sizeof(int),f);
	for(i=0;i<min(np,n);i++)
	{
		l=((int*)tmp)[2*i+1]; if(endianness==kINTEL) swapint(&l);
		an[i].x=(l&0xff)/255.0;
		an[i].y=((l>>8)&0xff)/255.0;
		an[i].z=((l>>16)&0xff)/255.0;
	}
	err=0;
			}
	free(tmp);
			}
	fclose(f);
	return err;
}
#pragma mark -
void initOpenGL(void)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_SMOOTH);
    glClearColor(back.x,back.y,back.z,1);
}
void depth(void)
{
	int			i;
    float		n,max;
	float3D		ce={0,0,0},ide,siz;
	
	// compute sulcal depth
	for(i=0;i<np;i++)
	{
		ce=(float3D){ce.x+p[i].x,ce.y+p[i].y,ce.z+p[i].z};
		
		if(i==0) ide=siz=p[i];
		
		if(ide.x<p[i].x) ide.x=p[i].x;
		if(ide.y<p[i].y) ide.y=p[i].y;
		if(ide.z<p[i].z) ide.z=p[i].z;
		
		if(siz.x>p[i].x) siz.x=p[i].x;
		if(siz.y>p[i].y) siz.y=p[i].y;
		if(siz.z>p[i].z) siz.z=p[i].z;
	}
	ce=(float3D){ce.x/(float)np,ce.y/(float)np,ce.z/(float)np};

	float	*sdepth=(float*)calloc(np,sizeof(float));
	max=0;
    for(i=0;i<np;i++)
    {
        n=	pow(2*(p[i].x-ce.x)/(ide.x-siz.x),2) +
			pow(2*(p[i].y-ce.y)/(ide.y-siz.y),2) +
			pow(2*(p[i].z-ce.z)/(ide.z-siz.z),2);

        sdepth[i] = sqrt(n);
        if(sdepth[i]>max)	max=sdepth[i];
    }
    max*=1.05;	// pure white is not nice...
    for(i=0;i<np;i++)
        sdepth[i]=sdepth[i]/max;
    for(i=0;i<np;i++)
    	an[i]=(float3D){an[i].x*sdepth[i],an[i].y*sdepth[i],an[i].z*sdepth[i]};
}
void centre(void)
{
	int	i;
	float	z=3;
	float3D	c={0,0,0};
	for(i=0;i<np;i++)
		c=(float3D){c.x+p[i].x,c.y+p[i].y,c.z+p[i].z};
	c=(float3D){c.x/(float)np,c.y/(float)np,c.z/(float)np};
	for(i=0;i<np;i++)
		p[i]=(float3D){z*(p[i].x-c.x),z*(p[i].y-c.y),z*(p[i].z-c.z)};
}
void drawSurface(int width, int height, float *rot, int toonFlag)
{
    int		i;
    float	zoom=1/kZoom;
	float3D	x,a,b,c,zero={0,0,0};
	float	aspectRatio=width/(float)height;
    
    // init projection
        glViewport(0, 0, (GLsizei)width, (GLsizei)height);
        glClear(GL_COLOR_BUFFER_BIT+GL_DEPTH_BUFFER_BIT+GL_STENCIL_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(zoom*width/2,-zoom*width/2,-zoom*height/2,zoom*height/2, -100000.0, 100000.0);

    // prepare drawing
        glMatrixMode (GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt (0,0,-10, 0,0,0, 0,1,0); // eye,center,updir
        glRotatef(rot[0],1,0,0);
        glRotatef(rot[1],0,1,0);
        glRotatef(rot[2],0,0,1);

    // draw
        glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3,GL_FLOAT,0,(GLfloat*)p);
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3,GL_FLOAT,0,(GLfloat*)an);
        glDrawElements(GL_TRIANGLES,nt*3,GL_UNSIGNED_INT,(GLuint*)t);

	// toon shading
		if(toonFlag)
		{
			glEnable( GL_CULL_FACE );
			glPolygonMode( GL_BACK, GL_FILL );
			glCullFace( GL_FRONT );

			glPolygonMode(GL_FRONT, GL_LINE);
			glLineWidth(3.0);
			glCullFace(GL_BACK);
			glDepthFunc(GL_LESS);
			glColor3f(0,0,0);
			glBegin(GL_TRIANGLES);
			for(i=0;i<nt;i++)
			{
				a=p[t[i].a];
				b=p[t[i].b];
				c=p[t[i].c];
				
				glVertex3fv((float*)&a);
				glVertex3fv((float*)&b);
				glVertex3fv((float*)&c);
			}
			glEnd();
			glDisable( GL_CULL_FACE );
		}

}
void getPixels(char *baseaddr, long width, long height)
{
    glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,baseaddr);
}
#pragma mark -
void WriteHexString(FILE *f, char *str)
{
	int		i,j,len=strlen(str);
	int		a;
	short	b;
	char	c[5],d[2];
	
	for(i=0;i<len;i+=4)
	{
		for(j=0;j<4;j++)
			c[j]=str[i+j];
		c[4]=(char)0;
		sscanf(c,"%x",&a);
		b=(short)a;
		fwrite(&((char*)&b)[1],1,1,f);
		fwrite(&((char*)&b)[0],1,1,f);
	}
}
void writeTIFF(char *path, char *addr, int nx, int ny)
{
	FILE	*fptr;
	int		offset;
	int		i,j;
	char	red,green,blue;
	
	fptr=fopen(path,"w");
	
	/* Write the header */
	WriteHexString(fptr,"4d4d002a");    /* Little endian & TIFF identifier */
	offset = nx * ny * 3 + 8;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);

	/* Write the binary data */
	for (j=0;j<ny;j++) {
	  for (i=0;i<nx;i++) {
	
		 red=addr[4*(j*nx+i)+0];
		 green=addr[4*(j*nx+i)+1];
		 blue=addr[4*(j*nx+i)+2];
		 
		 fputc(red,fptr);
		 fputc(green,fptr);
		 fputc(blue,fptr);
	  }
	}
   
	WriteHexString(fptr,"000e");  						/* Write the footer */ /* The number of directory entries (14) */
	WriteHexString(fptr,"0100000300000001");			/* Width tag, short int */
	fputc((nx & 0xff00) / 256,fptr);    /* Image width */
	fputc((nx & 0x00ff),fptr);
	WriteHexString(fptr,"0000");
	WriteHexString(fptr,"0101000300000001");			/* Height tag, short int */
	fputc((ny & 0xff00) / 256,fptr);    /* Image height */
	fputc((ny & 0x00ff),fptr);
	WriteHexString(fptr,"0000");
	WriteHexString(fptr,"0102000300000003");			/* Bits per sample tag, short int */
	offset = nx * ny * 3 + 182;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);
	WriteHexString(fptr,"010300030000000100010000");	/* Compression flag, short int */
	WriteHexString(fptr,"010600030000000100020000");	/* Photometric interpolation tag, short int */
	WriteHexString(fptr,"011100040000000100000008");	/* Strip offset tag, long int */
	WriteHexString(fptr,"011200030000000100010000");	/* Orientation flag, short int */
	WriteHexString(fptr,"011500030000000100030000");	/* Sample per pixel tag, short int */
	WriteHexString(fptr,"0116000300000001");			/* Rows per strip tag, short int */
	fputc((ny & 0xff00) / 256,fptr);
	fputc((ny & 0x00ff),fptr);
	WriteHexString(fptr,"0000");
	WriteHexString(fptr,"0117000400000001");			/* Strip byte count flag, long int */
	offset = nx * ny * 3;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);
	WriteHexString(fptr,"0118000300000003");			/* Minimum sample value flag, short int */
	offset = nx * ny * 3 + 188;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);
	WriteHexString(fptr,"0119000300000003");			/* Maximum sample value tag, short int */
	offset = nx * ny * 3 + 194;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);
	WriteHexString(fptr,"011c00030000000100010000");	/* Planar configuration tag, short int */
	WriteHexString(fptr,"0153000300000003");			/* Sample format tag, short int */
	offset = nx * ny * 3 + 200;
	putc((offset & 0xff000000) / 16777216,fptr);
	putc((offset & 0x00ff0000) / 65536,fptr);
	putc((offset & 0x0000ff00) / 256,fptr);
	putc((offset & 0x000000ff),fptr);
	WriteHexString(fptr,"00000000");					/* End of the directory entry */
	WriteHexString(fptr,"000800080008");				/* Bits for each colour channel */
	WriteHexString(fptr,"000000000000");				/* Minimum value for each component */
	WriteHexString(fptr,"00ff00ff00ff");				/* Maximum value per channel */
	WriteHexString(fptr,"000100010001");				/* Samples per pixel for each channel */
	fclose(fptr);
}
#pragma mark -
int main(int argc, char *argv[])
{
	// argv[1]	path to subject directory (one specific subject)
	// argv[2]	path to picture
	// argv[3]	orientation: lat, med

	char	subdir[2048]="";
	char	outtif[2048]="";
	char	ori[16]="";
	char	hem[16]="";
	char	meshpath[2048];
	char	annotpath[2048];
	long	width=kZoom*640;
	long	height=kZoom*480;
	float	rot[3]={90,0,-90};
	int		i,n;
	int		noann=0;
	int		toonFlag=0;
		
	// Default background color: black
	back=(float3D){0xff,0xff,0xff};

	// Get arguments
	n=0;
	for(i=0;i<argc;i++)
	{
		if(strcmp(argv[i],"-sub")==0){  strcpy(subdir,argv[++i]);	n++;}
		else
		if(strcmp(argv[i],"-out")==0){  strcpy(outtif,argv[++i]);	n++;}
		else
		if(strcmp(argv[i],"-ori")==0){  strcpy(ori,argv[++i]);		n++;}
		else
		if(strcmp(argv[i],"-hem")==0){  strcpy(hem,argv[++i]);		n++;}
		else
		if(strcmp(argv[i],"-back")==0){ sscanf(argv[++i],"%f,%f,%f",&back.x,&back.y,&back.z);}
		else
		if(strcmp(argv[i],"-noann")==0){noann=1;}
		else
		if(strcmp(argv[i],"-toon")==0){toonFlag=1;}
	}
	if(n<4)
	{
		printf("ERROR: missing mandatory arguments (sub, out, ori or hem).\n");
		return 1;
	}
	
	// Check endianness
	checkEndianness();
	
    // Init OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(width,height);
    glutCreateWindow("fsqc");
	initOpenGL();

	// Configure FreeSurfer mesh and annotation paths
	sprintf(meshpath,"%s/surf/%s.white",subdir,hem);
	sprintf(annotpath,"%s/label/%s.aparc.annot",subdir,hem);
	
	// Load mesh
	FreeSurfer_load_mesh(meshpath);
	centre();	// translate mesh to centre
	
	// Load annotation file (labels)
	an=(float3D*)calloc(np,sizeof(float3D));
	if(noann==1)
		for(i=0;i<np;i++)
			an[i]=(float3D){1,1,1};
	else
		FreeSurfer_load_annotation(annotpath);
	depth();	// multiply colours by sulcal depth

	// Rotation
	if((strcmp(hem,"lh")==0 && strcmp(ori,"lat")==0) || (strcmp(hem,"rh")==0 && strcmp(ori,"med")==0))
	{
		rot[0]=90;	rot[1]=0;	rot[2]=90;
	}
	else
	if((strcmp(hem,"lh")==0 && strcmp(ori,"med")==0) || (strcmp(hem,"rh")==0 && strcmp(ori,"lat")==0))
	{
		rot[0]=90;	rot[1]=0;	rot[2]=-90;
	}
	
	// OpenGL draw
	drawSurface(width,height,rot,toonFlag);
	//	glTranslatef(30, 30, 0); glutBitmapCharacter(GLUT_BITMAP_8_BY_13,65);

	// Write image in TIFF format
	char	*addr;
	addr=(char*)calloc(width*height,sizeof(char)*4);
	getPixels(addr,width,height);
	writeTIFF(outtif,addr,width,height);
	
	// Free allocated memory
	free(p);
	free(t);
	free(an);
	free(addr);
	
	return 0;
}
