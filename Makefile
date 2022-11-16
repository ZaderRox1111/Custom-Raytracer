all: raytrace

raytrace: Raycaster.c Raycaster.h raytrace.c v3math.c v3math.h
	gcc -o raytrace Raycaster.c raytrace.c v3math.c
