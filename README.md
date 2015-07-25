# fsqc
fsqc is a FreeSurfer Quality Control command line to create images to quality
control brain surface reconstructions generated using FreeSurfer.It produces tif
images with colour labels superimposed for a subject.

Usage example:
./fsqc -sub freesurfer/subjects/bert/ -out lh-toon.tif -ori lat -hem lh -toon

# Compilation

on Mac OS X:

    gcc -o fsqc fsqc.c -framework Carbon -framework OpenGL -framework GLUT

on Unix:

    gcc -o fsqc fsqc.c -lGL -lGLU -lglut

on Windows:

    gcc -o fsqc.exe fsqc.c -lopengl32 -lglut32

