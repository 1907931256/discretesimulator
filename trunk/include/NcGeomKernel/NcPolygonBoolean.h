#ifndef NC_POLYGONBOOLEAN
#define NC_POLYGONBOOLEAN

#include <stdio.h>
#include "NcGeomKernel\NcGeomKernelGlobal.h"
#include "NcUtility\NcGlobalDefinitions.h"



namespace DiscreteSimulator
{
	class vector2d
	{

	public:
		double v[2];

	};

	class line
	{
	public:
		vector2d		*start_pt;
		vector2d		*end_pt;
		
		line			*next;
		line			*prev;
	};



	class polygon
	{
	public:
		int			num_vertex;
		vector2d	*vertex;
		line		*start;
		line		*edge;

	};


	class NCGEOMKERNELEXPORT NcPolygonBoolean
	{
		
	public:
		NcPolygonBoolean();
		~NcPolygonBoolean();

		STATUS	boolean_main(double **dTargetPoly, int iTargetPoints, double **, int iToolPoints, double [][2], int &);
		STATUS	construct_polygon(polygon &poly, double **dInputPoly, int iPoint);
		STATUS	fill_edges(line *, vector2d *, int ,int );
		INT_STATUS intersect_line(line *p, line *q, double t[2]);
		STATUS	intersect_polygon(polygon &, polygon &);
		STATUS	polgon_boolean(polygon &, polygon );
		STATUS	find_tool_edge_tobeadded(polygon tool, polygon target, vector2d v1, vector2d v2, line *tobe_added[100], int &);
		STATUS	display_polygon(polygon, polygon, FILE *);
		int		pointInPolygon(polygon, vector2d );
		double	polarAngle(vector2d p);
		int		add_vertex(polygon &target, vector2d v);
		int		add_vertex(polygon &target, line edge, double t);
		STATUS	isSame(vector2d v1, vector2d v2);
		double	signedAngle(vector2d a, line l);
	};
}

#endif