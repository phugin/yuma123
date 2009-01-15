/*  FILE: cli.c

  Parse an XML representation of a parameter set instance,
  and create a ps_parmset_t struct that contains the data found.

*********************************************************************
*                                                                   *
*                  C H A N G E   H I S T O R Y                      *
*                                                                   *
*********************************************************************

date         init     comment
----------------------------------------------------------------------
21oct05      abb      begun
19dec05      abb      restart after refining NCX
10feb06      abb      change raw xmlTextReader interface to use
                      ses_cbt_t instead
10feb07      abb      split common routines from agt_ps_parse
                      so mgr code can use it
21jul08      abb      converted to cli.c; removed non-CLI functions

*********************************************************************
*                                                                   *
*                     I N C L U D E    F I L E S                    *
*                                                                   *
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <xmlstring.h>

#ifndef _H_procdefs
#include  "procdefs.h"
#endif

#ifndef _H_cfg
#include "cfg.h"
#endif

#ifndef _H_cli
#include "cli.h"
#endif

#ifndef _H_dlq
#include "dlq.h"
#endif

#ifndef _H_log
#include "log.h"
#endif

#ifndef _H_obj
#include "obj.h"
#endif

#ifndef _H_status
#include  "status.h"
#endif

#ifndef _H_typ
#include "typ.h"
#endif

#ifndef _H_val
#include "val.h"
#endif

#ifndef _H_val_util
#include "val_util.h"
#endif

#ifndef _H_var
#include "var.h"
#endif

#ifndef _H_xml_util
#include "xml_util.h"
#endif

#ifndef _H_yangconst
#include "yangconst.h"
#endif


/********************************************************************
*                                                                   *
*                       C O N S T A N T S                           *
*                                                                   *
*********************************************************************/

#define CLI_DEBUG 1

/********************************************************************
*                                                                   *
*                       V A R I A B L E S			    *
*                                                                   *
*********************************************************************/


/********************************************************************
* FUNCTION parse_parm
* 
* Create a ps_parm_t struct for the specified parm value,
* and insert it into the parmset (extended)
*
* INPUTS:
*   val == complex val_value_t to add the parsed parm into
*   obj == obj_template_t descriptor for the missing parm
*   strval == string representation of the object value
*             (may be NULL if obj btype is NCX_BT_EMPTY
*   script == TRUE if parsing a script (in the manager)
*          == FALSE if parsing XML or CLI
*
* OUTPUTS:
*   If the specified parm is mandatory w/defval defined, then a 
*   new ps_parm_t will be inserted in the ps->parmQ as required
*   to fill in the parmset.
*
* RETURNS:
*   status 
*********************************************************************/
static status_t
    parse_parm (val_value_t *val,
		const obj_template_t *obj,
		const xmlChar *strval,
		boolean script)
{
    val_value_t       *new_parm;
    const typ_def_t   *typdef;
    ncx_btype_t        btyp;
    status_t           res;
    
#ifdef DEBUG
    if (!val || !obj) {
	return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* create a new parm and fill it in */
    new_parm = val_new_value();
    if (!new_parm) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(new_parm, obj);

    /* get the base type value */
    btyp = obj_get_basetype(obj);

    if (typ_is_simple(btyp)) {
	if (script) {
	    (void)var_get_script_val(obj, new_parm,
				     strval, ISPARM, &res);
	} else {
	    typdef = obj_get_ctypdef(obj);
	    res = val_set_simval(new_parm,
				 typdef,  
				 obj_get_nsid(obj),
				 obj_get_name(obj),
				 strval);
	}
    } else {
	res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    /* save or free the new child node */
    if (res != NO_ERR) {
	val_free_value(new_parm);
    } else {
	val_add_child(new_parm, val);
    }
    return res;

}  /* parse_parm */


/********************************************************************
* FUNCTION parse_parm_ex
* 
* Create a ps_parm_t struct for the specified parm value,
* and insert it into the parmset (extended)
*
* INPUTS:
*   val == complex val_value_t to add the parsed parm into
*   obj == obj_template_t descriptor for the missing parm
*   nsid == namespace ID to really use
*   name == object name to really use
*   strval == string representation of the object value
*             (may be NULL if obj btype is NCX_BT_EMPTY
*   script == TRUE if parsing a script (in the manager)
*          == FALSE if parsing XML or CLI
*
* OUTPUTS:
*   If the specified parm is mandatory w/defval defined, then a 
*   new ps_parm_t will be inserted in the ps->parmQ as required
*   to fill in the parmset.
*
* RETURNS:
*   status 
*********************************************************************/
static status_t
    parse_parm_ex (val_value_t *val,
		   const obj_template_t *obj,
		   xmlns_id_t  nsid,
		   const xmlChar *name,
		   const xmlChar *strval,
		   boolean script)
{
    val_value_t       *new_parm;
    const typ_def_t   *typdef;
    ncx_btype_t        btyp;
    status_t           res;
    
#ifdef DEBUG
    if (!val || !obj) {
	return SET_ERROR(ERR_INTERNAL_PTR);
    }
#endif

    /* create a new parm and fill it in */
    new_parm = val_new_value();
    if (!new_parm) {
        return ERR_INTERNAL_MEM;
    }
    val_init_from_template(new_parm, obj);

    /* adjust namespace ID and name */
    val_set_name(new_parm, name, xml_strlen(name));
    new_parm->nsid = nsid;

    /* get the base type value */
    btyp = obj_get_basetype(obj);

    if (typ_is_simple(btyp)) {
	if (script) {
	    (void)var_get_script_val(obj, new_parm,
				     strval, ISPARM, &res);
	} else {
	    typdef = obj_get_ctypdef(obj);
	    res = val_set_simval(new_parm, typdef,  
				 nsid, name, strval);
	}
    } else {
	res = SET_ERROR(ERR_INTERNAL_VAL);
    }

    if (res != NO_ERR) {
	val_free_value(new_parm);
    } else {
	val_add_child(new_parm, val);
    }
    return res;

}  /* parse_parm_ex */


/**************    E X T E R N A L   F U N C T I O N S **********/


/********************************************************************
* FUNCTION cli_parse
* 
* Generate 1 val_value_t struct from a Unix Command Line,
* which should conform to the specified obj_template_t definition.
*
* For CLI interfaces, only one container object can be specified
* at this time.
*
* CLI Syntax Supported
*
* [prefix] parmname [separator] [value]
*
*    prefix ==  0 to 2 dashes    foo  -foo --foo
*    parmname == any valid NCX identifier string
*    separator ==  - equals char (=)
*                  - whitespace (sp, ht)
*    value == any NCX number or NCX string
*          == 'enum' data type
*          == not present (only for 'flag' data type)
*          == extended
*    This value: string will converted to appropriate 
*    simple type in val_value_t format.
*
* The format "--foo=bar" must be processed within one argv segment.
* If the separator is a whitespace char, then the next string
* in the argv array must contain the expected value.
* 
*
* DESIGN NOTES:
*
*   1) parse the (argc, argv) input against the specified object
*   2) add any missing mandatory and condition-met parameters
*      to the container. Defaults within a choice group are not
*      checked during this phase (unless valonly is TRUE)
*   3) Set the instance sequence IDs to distinguish duplicates
*   4) Validate any 'choice' constructs within the parmset
*   5) Validate the proper number of instances (missing or extra)
*      (unless valonly is TRUE)
*
* The 'value' string cannot be split across multiple argv segments.
* Use quotation chars within the CLI shell to pass a string
* containing whitespace to the CLI parser:
*
*   --foo="quoted string if whitespace needed"
*   --foo="quoted string if setting a variable \
*         as a top-level assignment"
*   --foo=unquoted-string-without-whitespace
*
* The input is parsed against the specified obj_template_t struct.
* A val_value_t tree is built as the input is read.
* Each parameter is syntax checked as is is parsed.
*
* If possible, the parser will skip to next parmameter in the parmset,
* in order to support 'continue-on-error' type of operations.
*
* Any agent callback functions that might be registered for
* the specified container (or its sub-nodes) are not called
* by this function.  This must be done after this function
* has been called, and returns NO_ERR.
* 
* !!! TO-DO !!!
*  - There are no 1-char aliases for NCX parameters.
*  - Position-dependent, unnamed parameters are not supported
*    at this time.
*
* INPUTS:
*   argc == number of strings passed in 'argv'
*   argv == array of command line argument strings
*   obj == obj_template_t of the container 
*          that should be used to validate the input
*          against the child nodes of this container
*   valonly == TRUE if only the values presented should
*             be checked, no defaults, missing parms (Step 1&3 only)
*           == FALSE if all the tests and procedures should be done
*   autocomp == TRUE if parameter auto-completion should be
*               tried if any specified parameters are not matches
*               for the specified parmset
*            == FALSE if exact match only is desired
*   status == pointer to status_t to get the return value
*
* OUTPUTS:
*   *status == the final function return status

* Just as the NETCONF parser does, the CLI parser will not add a parameter
* to the val_value_t if any errors occur related to the initial parsing.
*
* RETURNS:
*   pointer to the malloced and filled in val_value_t
*********************************************************************/
val_value_t *
    cli_parse (int argc, 
	       const char *argv[],
	       const obj_template_t *obj,
	       boolean valonly,
	       boolean script,
	       boolean autocomp,
	       status_t  *status)
{
    val_value_t    *val;
    const obj_template_t *chobj;
    const char     *msg;
    char           *parmname, *parmval, *str, *buff;
    int32           parmnum;
    uint32          parmnamelen, buffpos, bufflen;
    ncx_btype_t     btyp;
    status_t        res;
    xmlChar         errbuff[NCX_MAX_NLEN];

#ifdef DEBUG
    if (!status) {
	SET_ERROR(ERR_INTERNAL_PTR);
	return NULL;
    }
    if (!argv || !obj) {
	*status = SET_ERROR(ERR_INTERNAL_PTR);
	return NULL;
    }
#endif

    /* check if CLI parmset really OK to use
     * Must have only choices, leafs, and leaflists
     * Since the CLI parameters come from an external YANG file
     * This is a quick check to make sure the CLI code will work
     * with the provided object template.
     */
    if (!script && !obj_ok_for_cli(obj)) {
	*status = SET_ERROR(ERR_NCX_OPERATION_FAILED);
	return NULL;
    }

    /* check if there are any parameters at all to parse */
    if (argc < 2) {
	*status = NO_ERR;
	return NULL;
    }


    /* allocate a new container value to hold the output */
    val = val_new_value();
    if (!val) {
	*status = ERR_INTERNAL_MEM;
	return NULL;
    }
    val_init_from_template(val, obj);

    if (script) {
	/* set the parmset name to the application pathname */
	val->dname = xml_strdup((const xmlChar *)argv[0]);
	val->name = val->dname;
    } else {
	/* set the parmset name to the PSD static name */
	val->name = obj_get_name(obj);
    }

    /* check for a malloc error */
    if (!val->name) {
	*status = ERR_INTERNAL_MEM;
	val_free_value(val);
	return NULL;
    }

    /* gather all the argv strings into one buffer for easier parsing
     * need to copy because the buffer is written during parsing
     * and the argv parameter is always a 'const char **' data type
     *
     * first determine the buffer length
     */
    bufflen = 0;
    for (parmnum=1; parmnum < argc; parmnum++) {
	bufflen += (uint32)(strlen(argv[parmnum]) + 1);
    }

    /* adjust extra space */
    if (bufflen) {
	bufflen--;
    }

    buff = m__getMem(bufflen+1);
    if (!buff) {
	/* non-recoverable error */
	*status = ERR_INTERNAL_MEM;
	val_free_value(val);
	return NULL;
    }

    /* copy the argv strings into the buffer */
    buffpos = 0;
    for (parmnum=1; parmnum < argc; parmnum++) {
	buffpos += xml_strcpy((xmlChar *)&buff[buffpos], 
			      (const xmlChar *)argv[parmnum]);
	if (parmnum+1 < argc) {
	    buff[buffpos++] = ' ';
	}
    }
    buff[buffpos] = 0;

    /* setup parm loop */
    buffpos = 0;

    /* 1) go through all the command line strings
     * setup parmname and parmval based on strings found
     * and the PSD for these parameters
     * save each parm value in a ps_parm_t struct
     */
    while (buffpos < bufflen) {

	res = NO_ERR;

	/* first skip starting whitespace */
	while (buff[buffpos] && isspace(buff[buffpos])) {
	    buffpos++;
	}

	/* check start of parameter name conventions 
	 * allow zero, one, or two dashes before parmname
         *          foo   -foo   --foo
	 */
	if (!buff[buffpos]) {
	    res = ERR_NCX_WRONG_LEN;
	} else if (buff[buffpos] == NCX_CLI_START_CH) {
	    if (!buff[buffpos+1]) {
		res = ERR_NCX_WRONG_LEN;
	    } else if (buff[buffpos+1] == NCX_CLI_START_CH) {
		if (!buff[buffpos+2]) {
		    res = ERR_NCX_WRONG_LEN;
		} else {
		    buffpos += 2;  /* skip past 2 dashes */
		}
	    } else {
		buffpos++;    /* skip past 1 dash */
	    }
	} /* else no dashes, leave parmname pointer alone */

	/* check for the end of the parm name, wsp or equal sign
	 * get the parm template, and get ready to parse it
	 */
	if (res == NO_ERR) {
	    /* check the parmname string for a terminating char */
	    parmname = &buff[buffpos];
	    str = &parmname[1];
	    while (*str && !isspace(*str) && *str != NCX_ASSIGN_CH) {
		str++;
	    }

	    parmnamelen = (uint32)(str - parmname);
	    buffpos += parmnamelen;

	    /* check if this parameter name is in the parmset def */
	    chobj = obj_find_child_str(obj, obj_get_mod_name(obj),
				       (const xmlChar *)parmname,
				       parmnamelen);

	    /* check if parm was found, try partial name if not */
	    if (!chobj && autocomp) {
		chobj = 
		    obj_match_child_str(obj, obj_get_mod_name(obj),
					(const xmlChar *)parmname,
					parmnamelen);
	    }
	    
	    if (!chobj) {
		res = ERR_NCX_UNKNOWN_PARM;
	    } else {
		/* do not check parameter order for CLI */
		btyp = obj_get_basetype(chobj);
		parmval = NULL;

		/* skip past any whitespace after the parm name */
		while (isspace(buff[buffpos]) && buffpos < bufflen) {
		    buffpos++;
		}

		/* check if ended on space of EOLN */
		if (btyp == NCX_BT_EMPTY) {
		    if (buff[buffpos] == '=') {
			log_error("\nError: cannot assign value "
				  "to empty leaf '%s'", 
				  obj_get_name(obj));
			res = ERR_NCX_INVALID_VALUE;
		    }
		} else if (buffpos < bufflen) {
		    if (buff[buffpos] == '=') {
			buffpos++;

			/* skip any whitespace */
			while (buff[buffpos] && isspace(buff[buffpos])) {
			    buffpos++;
			}
		    } /* else whitespace already skipped */

		    /* if any chars left in buffer, get the parmval */
		    if (buffpos < bufflen) {
			if (buff[buffpos] == NCX_QUOTE_CH) {
			    if (script) {
				/* set the start at quote */
				parmval = &buff[buffpos];
			    } else {
				/* set the start after quote */
				parmval = &buff[++buffpos];
			    }

			    /* find the end of the quoted string */
			    str = &parmval[1];
			    while (*str && *str != NCX_QUOTE_CH) {
				str++;
			    }
			    /* if script mode keep ending quote */
			    if (script && *str == NCX_QUOTE_CH) {
				str++;
			    }
			} else if (script && (buffpos+1 < bufflen) &&
				   (buff[buffpos] == NCX_XML1a_CH) &&
				   (buff[buffpos+1] == NCX_XML1b_CH)) {
			    /* set the start of the XML parmval to the [ */
			    parmval = &buff[buffpos];
			    
			    /* find the end of the inline XML */
			    str = parmval+1;
			    while (*str && !((*str==NCX_XML2a_CH) 
					     && (str[1]==NCX_XML2b_CH))) {
				str++;
			    }

			    if (!*str) {
				/* did not find end of XML string */
				res = ERR_NCX_DATA_MISSING;
			    } else {
				/* setup after the ] char to be zeroed */
				str += 2;
			    }
			} else {
			    /* set the start of the parmval */
			    parmval = &buff[buffpos];

			    /* find the end of the unquoted string */
			    str = &parmval[1];
			    while (*str && !isspace(*str)) {
				str++;
			    }
			}

			/* terminate string */
			*str = 0;

			/* skip buffpos past eo-string */
			buffpos += (uint32)((str - parmval) + 1);  
		    }
		}

		/* make sure value entered if expected */
		if (res==NO_ERR && !parmval && btyp != NCX_BT_EMPTY) {
		    res = ERR_NCX_EMPTY_VAL;
		}
	    }
        }

	/* create a new val_value struct and set the value */
	if (res == NO_ERR) {
	    res = parse_parm(val, chobj, 
			     (const xmlChar *)parmval,
			     script);
	}

	/* check any errors in the parm name or value */
	if (res != NO_ERR) {
	    msg = get_error_string(res);
	    xml_strncpy(errbuff, (const xmlChar *)parmname, parmnamelen);
	    switch (res) {
	    case ERR_NCX_UNKNOWN_PARM:
		log_error("\nError: Unknown parameter (%s)", errbuff);
		break;
	    default:
		if (buffpos < bufflen) {
		    log_error("\nError: %s (%s = %s)", 
			      msg, errbuff, &buff[buffpos]);
		} else if (parmval) {
		    log_error("\nError: %s (%s = %s)", msg, errbuff, parmval);
		} else {
		    log_error("\nError: %s (%s)", msg, errbuff);
		}
	    }
	    ncx_print_errormsg(NULL, NULL, res);
	    m__free(buff);
	    *status = res;
	    return val;
	}
    }

    /* cleanup after loop */
    m__free(buff);
    buff = NULL;
    res = NO_ERR;

    /* 2) add any defaults for mandatory parms that are not set */
    if (!valonly) {
	res = val_add_defaults(val, script);
    }

    /* 3) CLI Instance Check
     * Go through all the parameters in the object and check
     * to see if each child node is present with the 
     * correct number of instances
     * The CLI object node can be a choice or a simple parameter 
     * The choice test is also performed by this function call
     */
    if (res == NO_ERR && !valonly) {
	res = val_instance_check(val, val->obj);
    }

    *status = res;
    return val;

}  /* cli_parse */


/********************************************************************
* FUNCTION cli_parse_parm
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the parent container value
*
* ONLY CALLED FROM CLI PARSING FUNCTIONS IN ncxcli.c
* ALLOWS SCRIPT EXTENSIONS TO BE PRESENT
*
* INPUTS:
*   val == parent value struct to adjust
*   parm == obj_template_t descriptor for the missing parm
*   strval == string representation of the parm value
*             (may be NULL if parm btype is NCX_BT_EMPTY
*   script == TRUE if CLI script mode
*          == FALSE if CLI plain mode
* 
* OUTPUTS:
*   A new val_value_t will be inserted in the val->v.childQ 
*   as required to fill in the parm.
*
* RETURNS:
*   status 
*********************************************************************/
status_t
    cli_parse_parm (val_value_t *val,
		    const obj_template_t *obj,
		    const xmlChar *strval,
		    boolean script)
{
    return parse_parm(val, obj, strval, script);

}  /* cli_parse_parm */


/********************************************************************
* FUNCTION cli_parse_parm_ex
* 
* Create a val_value_t struct for the specified parm value,
* and insert it into the parent container value
* Allow different bad data error handling vioa parameter
*
* ONLY CALLED FROM CLI PARSING FUNCTIONS IN ncxcli.c
* ALLOWS SCRIPT EXTENSIONS TO BE PRESENT
*
* INPUTS:
*   val == parent value struct to adjust
*   parm == obj_template_t descriptor for the missing parm
*   strval == string representation of the parm value
*             (may be NULL if parm btype is NCX_BT_EMPTY
*   script == TRUE if CLI script mode
*          == FALSE if CLI plain mode
*   bad_data == enum defining how bad data should be handled
*
* OUTPUTS:
*   A new val_value_t will be inserted in the val->v.childQ 
*   as required to fill in the parm.
*
* RETURNS:
*   status 
*********************************************************************/
status_t
    cli_parse_parm_ex (val_value_t *val,
		       const obj_template_t *obj,
		       const xmlChar *strval,
		       boolean script,
		       ncx_bad_data_t  bad_data)
{
    const obj_template_t  *genstr;
    status_t               res;

    res = parse_parm(val, obj, strval, script);
    if (res == NO_ERR || NEED_EXIT(res)) {
	return res;
    }

    switch (bad_data) {
    case NCX_BAD_DATA_WARN:
	log_warn("\nWarning: invalid value '%s' used for parm '%s'",
		 (strval) ? strval : EMPTY_STRING,
		 obj_get_name(obj));
	/* drop through */
    case NCX_BAD_DATA_IGNORE:
	genstr = ncx_get_gen_string();
	res = parse_parm_ex(val, genstr, 
			    obj_get_nsid(obj),
			    obj_get_name(obj),
			    strval, script);
	return res;
    case NCX_BAD_DATA_CHECK:
    case NCX_BAD_DATA_ERROR:
	return res;
    default:
	return SET_ERROR(ERR_INTERNAL_VAL);
    }
    /*NOTREACHED*/

}  /* cli_parse_parm_ex */


/* END file cli.c */
