/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* devWfJoergerVtr1.c */
/* base/src/dev $Id$ */

/* devWfJoergerVtr1.c - Device Support Routines */
/*
 *      Original Author: Bob Dalesio
 *      Current Author:  Marty Kraimer
 *      Date:            6-1-90
 */


#include	<vxWorks.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<alarm.h>
#include	<dbDefs.h>
#include	<epicsPrint.h>
#include	<dbAccess.h>
#include        <recSup.h>
#include	<devSup.h>
#include	<dbScan.h>
#include	<link.h>
#include	<module_types.h>
#include	<waveformRecord.h>

#include	<drvJgvtr1.h>

static long init_record();
static long read_wf();
static long arm_wf();


struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
        DEVSUPFUN       get_ioint_info;
	DEVSUPFUN	read_wf;
} devWfJoergerVtr1={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	read_wf};


static void myCallback(pwf,no_read,pdata)
    struct waveformRecord   *pwf;
    int		    no_read;
    unsigned short   *pdata;
{
	struct rset     *prset=(struct rset *)(pwf->rset);
	short ftvl = pwf->ftvl;
	long   i;

	if(!pwf->busy) return;
        dbScanLock((struct dbCommon *)pwf);
	pwf->busy = FALSE;
	if(no_read>pwf->nelm)no_read = pwf->nelm;
	if(ftvl==DBF_CHAR || ftvl==DBF_UCHAR) {
		unsigned char *pdest=(unsigned char *)pwf->bptr;

		for(i=0; i<no_read; i++) {
			*pdest++ = *pdata++;
		}
       		pwf->nord = no_read;            /* number of values read */
	} else if(ftvl==DBF_SHORT || ftvl==DBF_USHORT) {
		unsigned short *pdest=(unsigned short *)pwf->bptr;

		for(i=0; i<no_read; i++) {
			*pdest++ = *pdata++;
		}
       		pwf->nord = no_read;            /* number of values read */
	} else {
		recGblRecordError(S_db_badField,(void *)pwf,
			"read_wf - illegal ftvl");
                recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
	}
	(*prset->process)(pwf);
        dbScanUnlock((struct dbCommon *)pwf);
}

static long init_record(pwf)
    struct waveformRecord	*pwf;
{

    /* wf.inp must be an VME_IO */
    switch (pwf->inp.type) {
    case (VME_IO) :
	break;
    default :
	recGblRecordError(S_db_badField,(void *)pwf,
		"devWfJoergerVtr1 (init_record) Illegal INP field");
	return(S_db_badField);
    }
    return(0);
}

static long read_wf(pwf)
    struct waveformRecord	*pwf;
{

	/* determine if wave form is to be rearmed*/
	/* If not active then request rearm */
	if(!pwf->pact) arm_wf(pwf);
	/* if already active then call is from myCallback. check rarm*/
	else if(pwf->rarm) {
		(void)arm_wf(pwf);
	}
	return(0);
}

static long arm_wf(pwf)
struct waveformRecord   *pwf;
{
	long	status;
	struct vmeio *pvmeio = (struct vmeio *)&(pwf->inp.value);

	pwf->busy = TRUE;
	status = jgvtr1_driver(pvmeio->card,myCallback,pwf);
	if(status!=0){
		errMessage(status, NULL);
                recGblSetSevr(pwf,READ_ALARM,INVALID_ALARM);
		pwf->busy = FALSE;
		return(0);
	}
	pwf->pact=TRUE;
	return(0);
}
