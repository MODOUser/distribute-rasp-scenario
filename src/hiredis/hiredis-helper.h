#include<hiredis/hiredis.h>
int
hiredis_zrange(const char *key, int start, int stop, int withscores)
{
	/* resp should be a barr of (bstr_t *). Caller responsible for freeing
	 * the returned elements. */
	int		err;
	redisReply	*r;
	redisReply	*elem;
	int		i;

	if(rctx == NULL)
		return ENOEXEC;
	err = 0;
	r = NULL;

	r = _redisCommand(withscores==0?ZRANGE_FMT:ZRANGE_WITHSCORES_FMT,
	    key, start, stop);

	if(r == NULL) {
		//blogf("Error while sending command to redis: NULL reply");
		err = ENOEXEC;
        return -1;
	} else
	if(r->type == REDIS_REPLY_ERROR) {
//		if(!xstrempty(r->str)) {
//			//blogf("Error while sending command to redis: %s",
//			    r->str);
//		} else {
//			//blogf("Error while sending command to redis,"
//			    " and no error string returned by redis!");
//		}

		err = ENOEXEC;
        return -1;

	} else
	if(r->elements == 0) {
		err = ENOENT;
        return -1;
	} else
	 if(r->type == REDIS_REPLY_ARRAY && r->element != NULL) {

		for(i = 0; i < r->elements; ++i) {
			elem = r->element[i];
			if(elem->type != REDIS_REPLY_STRING) {
				//blogf("Element is not string");
				continue;
			}
			if(xstrempty(elem->str)) {
				//blogf("Element is invalid string");
				continue;
			}
			str = binit();
			if(str == NULL) {
				//blogf("Couldn't allocate str");
				continue;
			}
			bstrcat(str, elem->str);
			barr_add(resp, (void *) str);
			free(str);
			str = NULL;
		}
	} else {
		//blogf("Redis didn't respond with valid array");
		err = ENOEXEC;
		goto end_label;
	}

end_label:

	if(r != NULL) {
		freeReplyObject(r);
		r = NULL;
	}

	return err;
}

