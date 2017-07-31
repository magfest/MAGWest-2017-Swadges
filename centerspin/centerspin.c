#include <math.h>
#include <stdio.h>

//NOTE: Units in MM

void MakeSpin( double angstart, double angend, double angent, double radiuso, double radiusi, int fangs )
{
	double epsilon = 0.01;
	double fang;
	int i;
	printf( "  (fp_poly (pts " );
//		printf( "(xy 0 0) (xy 0 1) (xy 1 1) (xy 1 0)\n" );

	//Top edge
	for( fang = angstart; fang <= angend; fang+=epsilon )
	{
		printf( "(xy %f %f) ",sin( fang ) * radiuso, cos(fang) * radiuso );
	}

	for( i = 0; i < fangs; i++ )
	{
		double delrad = (radiuso - radiusi)/fangs;
		double rad = radiuso - delrad * i;
		int steps = (int)(angent / epsilon);
		double delradep = delrad / steps / 2.0;
		for( fang = angend; fang >= (angend-angent); fang-=epsilon )
		{
			printf( "(xy %f %f) ",sin( fang ) * rad, cos(fang) * rad );
			rad -= delradep;
		}
		for( fang = (angend-angent); fang <= angend; fang+=epsilon )
		{
			printf( "(xy %f %f) ",sin( fang ) * rad, cos(fang) * rad );
			rad -= delradep;
		}
	}

	//Bottom edge
	for( fang = angend; fang >= angstart; fang-=epsilon )
	{
		printf( "(xy %f %f) ",sin( fang ) * radiusi, cos(fang) * radiusi );
	}


	for( i = 0; i < fangs; i++ )
	{
		double delrad = (radiusi - radiuso)/fangs;
		double rad = radiusi - delrad * i;
		int steps = (int)(angent / epsilon);
		double delradep = delrad / steps / 2.0;
		for( fang = angstart; fang >= (angstart-angent); fang-=epsilon )
		{
			printf( "(xy %f %f) ",sin( fang ) * rad, cos(fang) * rad );
			rad -= delradep;
		}
		for( fang = (angstart-angent); fang <= angstart; fang+=epsilon )
		{
			printf( "(xy %f %f) ",sin( fang ) * rad, cos(fang) * rad );
			rad -= delradep;
		}
	}


	printf( "  ) (layer F.Cu) (width 0.01))\n" );
/*    (fp_poly (pts 
*/
}

int main()
{

	printf( "(module Centertouch (layer F.Cu) (tedit 597EA18B)\n\
  (fp_text reference REF** (at 5.75 -1) (layer F.SilkS)\n\
    (effects (font (size 1 1) (thickness 0.15)))\n\
  )\n\
  (fp_text value Centertouch (at 6.25 -3) (layer F.Fab)\n\
    (effects (font (size 1 1) (thickness 0.15)))\n\
  )\n");

	MakeSpin( 0*3.14159/180.0, 100*3.14159/180.0, 120 * 3.14159 / 180.0, 28.0, 20.3, 2 );
	MakeSpin( 120*3.14159/180.0, 220*3.14159/180.0, 120 * 3.14159 / 180.0, 28.0, 20.3, 2 );
	MakeSpin( 240*3.14159/180.0, 340*3.14159/180.0, 120 * 3.14159 / 180.0, 28.0, 20.3, 2 );

	printf( ")\n" );
}


