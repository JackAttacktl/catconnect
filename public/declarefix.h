#ifndef __DECLAREFIX_
#define __DECLAREFIX_

#ifdef DECLARE_CLASS_SIMPLE
#undef DECLARE_CLASS_SIMPLE
#endif 

#define DECLARE_CLASS_SIMPLE( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass;	\
public:	\
	~className() {}

#endif