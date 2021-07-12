#ifndef __Z64AUDIO_LIB_H__
#define __Z64AUDIO_LIB_H__

#include "z64snd.h"

s16 Math_SmoothStepToS(s16* pValue, s16 target, s16 scale, s16 step, s16 minStep) {
	s16 stepSize = 0;
	s16 diff = target - *pValue;
	
	if (*pValue != target) {
		stepSize = diff / scale;
		
		if ((stepSize > minStep) || (stepSize < -minStep)) {
			if (stepSize > step) {
				stepSize = step;
			}
			
			if (stepSize < -step) {
				stepSize = -step;
			}
			
			*pValue += stepSize;
		} else {
			if (diff >= 0) {
				*pValue += minStep;
				
				if ((s16)(target - *pValue) <= 0) {
					*pValue = target;
				}
			} else {
				*pValue -= minStep;
				
				if ((s16)(target - *pValue) >= 0) {
					*pValue = target;
				}
			}
		}
	}
	
	return diff;
}

#endif