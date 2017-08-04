#include <math.h>
#include <stdio.h>

//NOTE: Units in MM

int main()
{
	printf( "(module Halftone (layer F.Cu) (tedit 5983FDC5)\n");

	int countx = 50;
	int county = 50;
	double radius0 = 20;
	double radius1 = 30;

	double spacing = 2.5;
	int x, y;
	for( y = 0; y < county; y++ )
	for( x = 0; x < countx; x++ )
	{
		double fx = (- countx / 2 + x) * spacing;
		double fy = (- county / 2 + y) * spacing;

		double distance = sqrt( fx*fx+fy*fy );
		if( distance <= radius0 ) continue;
		if( distance >= radius1 ) continue;
		double radius = distance - radius0;

		fprintf( stderr, "Rad: %f %f %f %f %f\n", radius, distance, (radius1-radius0)/2, fx, fy );
		if( radius > (radius1-radius0)/2 ) radius = (radius1-radius0) -radius;
		radius = pow( radius, 0.5 )* .5 + 1;
		
		printf( "(pad ~ smd oval (at %f %f) (size %f %f) (layers F.Cu F.Mask))\n", fx,fy, radius, radius );
	}



	printf( ")\n" );
}


