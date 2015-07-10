#include "ParamDef.h"

using namespace thp;

TBundleID::TBundleID() 
{
	unLv = 0;
	unBundleIDinLv = 0;
}

bool TBundleID::operator ==(const TBundleID& tNoNew) const
{
	if(tNoNew.unLv == unLv && unBundleIDinLv == tNoNew.unBundleIDinLv)
		return true;

	return false;
}

bool TBundleID::operator <(const TBundleID& tNoNew) const
{
	if( unLv < tNoNew.unLv )
		return true;

	if(unLv == tNoNew.unLv && unBundleIDinLv < tNoNew.unBundleIDinLv)
		return true;

	return false;
}


