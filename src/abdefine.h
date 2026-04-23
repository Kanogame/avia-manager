/* Define header for application - AppBuilder 2.03  */

/* 'base' Window link */
extern const int ABN_base;
#define ABW_base                             AbGetABW( ABN_base )
extern const int ABN_ActivePlanesList;
#define ABW_ActivePlanesList                 AbGetABW( ABN_ActivePlanesList )
extern const int ABN_ActivePlanesLabel;
#define ABW_ActivePlanesLabel                AbGetABW( ABN_ActivePlanesLabel )
extern const int ABN_PlaneSelection;
#define ABW_PlaneSelection                   AbGetABW( ABN_PlaneSelection )
extern const int ABN_PlaneX;
#define ABW_PlaneX                           AbGetABW( ABN_PlaneX )
extern const int ABN_PlaneY;
#define ABW_PlaneY                           AbGetABW( ABN_PlaneY )
extern const int ABN_PlaneChangeCourse;
#define ABW_PlaneChangeCourse                AbGetABW( ABN_PlaneChangeCourse )
extern const int ABN_TopView;
#define ABW_TopView                          AbGetABW( ABN_TopView )
extern const int ABN_AltView;
#define ABW_AltView                          AbGetABW( ABN_AltView )

#define AbGetABW( n ) ( AbWidgets[ n ].wgt )

#define AB_OPTIONS "s:x:y:h:w:S:"
