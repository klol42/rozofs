/*
  Copyright (c) 2010 Fizians SAS. <http://www.fizians.com>
  This file is part of Rozofs.

  Rozofs is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation, version 2.

  Rozofs is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
 */
 
#ifndef ROZOFS_FUSE_API_H
#define ROZOFS_FUSE_API_H

#include <rozofs/common/profile.h>
#include <rozofs/rozofs_timer_conf.h>
#include <rozofs/core/ruc_common.h>

#include "rozofs_fuse.h"

extern rozofs_fuse_save_ctx_t *rozofs_fuse_usr_ctx_table[];
extern uint32_t rozofs_fuse_usr_ctx_idx ;
extern uint64_t rozofs_write_merge_stats_tab[];


typedef enum {
   rozofs_trc_type_def = 0,
   rozofs_trc_type_io ,
   rozofs_trc_type_name,
   rozofs_trc_type_attr
} rozofs_trc_type_e;


#define ROZOFS_TRC_NAME_MAX 32
typedef struct _rozofs_trc_name_t
{
   char name[32];
} rozofs_trc_name_t;

typedef struct _rozofs_trc_io_t
{
  fid_t   fid;     /**< unique file identifier */
  off_t   off;     /**< off within the file    */
  size_t  size;    /**< size in bytes          */
} rozofs_trc_io_t;

typedef struct _rozofs_trc_attr_t
{
  fid_t     fid;     /**< unique file identifier */
  uint32_t  mode;     /**< off within the file    */
  uint64_t  size;    /**< size in bytes          */
} rozofs_trc_attr_t;

typedef struct _rozofs_trc_def_t
{
  fid_t   fid;     /**< unique file identifier */
  off_t   off;     /**< off within the file    */
  size_t  size;    /**< size in bytes          */
} rozofs_trc_def_t;

#define ROZOFS_TRACE_BUF_SZ 128
typedef union 
{
   uint32_t u32;
   struct {
   uint32_t req:1 ;         /**< assert to one if it is request , 0 for the response */
   uint32_t service_id:7;   /**< service identifier */
   uint32_t status:1;       /**< 1: success/ 0: failure */
   uint32_t fid:1;          /**< 1: if fid is present */
   uint32_t trc_type:3;           /**< see  rozofs_trc_type_e */
   uint32_t index:20 ;      /**< index of the request in the trace buffer */
   } s;
} rozofs_trace_hdr_t;

typedef struct _rozofs_trace_t
{
  rozofs_trace_hdr_t hdr;; /**< service identifier  */
  int      errno_val;
  uint64_t ts;
  fuse_ino_t ino;  /**< operation inode        */
  union
  {
    rozofs_trc_name_t name;
    rozofs_trc_io_t io;
    rozofs_trc_def_t def;
    rozofs_trc_attr_t attr;  
  } par;
} rozofs_trace_t;  


typedef enum {
	srv_rozofs_ll_lookup=0,
	srv_rozofs_ll_forget,
	srv_rozofs_ll_getattr,
	srv_rozofs_ll_setattr,
	srv_rozofs_ll_readlink,
	srv_rozofs_ll_mknod,
	srv_rozofs_ll_mkdir,
	srv_rozofs_ll_unlink,
	srv_rozofs_ll_rmdir,
	srv_rozofs_ll_symlink,
	srv_rozofs_ll_rename,
	srv_rozofs_ll_open,
	srv_rozofs_ll_link,
	srv_rozofs_ll_read,
	srv_rozofs_ll_write,
	srv_rozofs_ll_flush,
	srv_rozofs_ll_release,
	srv_rozofs_ll_opendir,
	srv_rozofs_ll_readdir,
	srv_rozofs_ll_releasedir,
	srv_rozofs_ll_fsyncdir,
	srv_rozofs_ll_statfs,
	srv_rozofs_ll_setxattr,
	srv_rozofs_ll_getxattr,
	srv_rozofs_ll_listxattr,
	srv_rozofs_ll_removexattr,
	srv_rozofs_ll_access,
	srv_rozofs_ll_create,
	srv_rozofs_ll_getlk,
	srv_rozofs_ll_setlk,
	srv_rozofs_ll_setlk_int,
	srv_rozofs_ll_ioctl,
	srv_rozofs_ll_clearlkowner,
} rozofs_service_e;

extern int rozofs_trc_wr_idx;
extern int rozofs_trc_buf_full;
extern int rozofs_trc_last_idx;
extern int rozofs_trc_enabled;
extern int rozofs_trc_index;
extern rozofs_trace_t *rozofs_trc_buffer;
/*
**____________________________________________________
*/
static inline int rozofs_trc_req(int service,fuse_ino_t ino,fid_t fid)
{
   rozofs_trace_t *p;
   if (rozofs_trc_enabled == 0) return 0;
   {
     p = &rozofs_trc_buffer[rozofs_trc_wr_idx];
     p->hdr.u32 = 0;
     p->ts = ruc_rdtsc();
     p->hdr.s.service_id = service;
     p->hdr.s.req = 1;
     p->hdr.s.trc_type  = rozofs_trc_type_def;
     p->hdr.s.index = rozofs_trc_index++;
     p->ino= ino;
     if (fid != NULL) 
     {
        memcpy(p->par.def.fid,fid,sizeof(fid_t)); 
	p->hdr.s.fid = 1;
     }   
     rozofs_trc_wr_idx++;
     if (rozofs_trc_wr_idx >= rozofs_trc_last_idx) 
     {
        rozofs_trc_wr_idx= 0;
	rozofs_trc_buf_full = 1;
     }
   }
   return (int) p->hdr.s.index;
}
/*
**____________________________________________________
*/

static inline int rozofs_trc_req_io(int service,fuse_ino_t ino,fid_t fid,size_t size,off_t off)
{
   rozofs_trace_t *p;
   if (rozofs_trc_enabled == 0) return 0;
   {
     p = &rozofs_trc_buffer[rozofs_trc_wr_idx];
     p->hdr.u32 = 0;
     p->ts = ruc_rdtsc();
     p->hdr.s.service_id = service;
     p->hdr.s.req = 1;
     p->hdr.s.trc_type  = rozofs_trc_type_io;
     p->hdr.s.index = rozofs_trc_index++;
     p->ino= ino;
     if (fid != NULL) 
     {
        memcpy(p->par.io.fid,fid,sizeof(fid_t)); 
	p->hdr.s.fid = 1;
     }   
     p->par.io.size = size;
     p->par.io.off  = off;
     rozofs_trc_wr_idx++;
     if (rozofs_trc_wr_idx >= rozofs_trc_last_idx) 
     {
        rozofs_trc_wr_idx= 0;
	rozofs_trc_buf_full = 1;
     }
   }
   return (int) p->hdr.s.index;
}


static inline int rozofs_trc_req_name(int service,fuse_ino_t ino,char *name)
{
   rozofs_trace_t *p;
   if (rozofs_trc_enabled == 0) return 0;
   {
     p = &rozofs_trc_buffer[rozofs_trc_wr_idx];
     p->hdr.u32 = 0;
     p->ts = ruc_rdtsc();
     p->hdr.s.service_id = service;
     p->hdr.s.req = 1;
     p->hdr.s.trc_type  = rozofs_trc_type_name;
     p->hdr.s.index = rozofs_trc_index++;
     p->ino= ino;     
     if (name != NULL) 
     {
        memset(p->par.name.name,0,sizeof(rozofs_trc_name_t));
        memcpy(p->par.name.name,name,sizeof(rozofs_trc_name_t)-1); 
     }
     rozofs_trc_wr_idx++;
     if (rozofs_trc_wr_idx >= rozofs_trc_last_idx) 
     {
        rozofs_trc_wr_idx= 0;
	rozofs_trc_buf_full = 1;
     }
   }
   return (int) p->hdr.s.index;
}
/*
**____________________________________________________
*/
#if 1
static inline void rozofs_trc_rsp(int service,fuse_ino_t ino,fid_t fid,int status,int index)
{
   rozofs_trace_t *p;
   if (rozofs_trc_enabled == 0) return;
   {
     
     p = &rozofs_trc_buffer[rozofs_trc_wr_idx];
     p->hdr.u32 = 0;
     p->ts = ruc_rdtsc();
     p->hdr.s.service_id = service;
     p->hdr.s.trc_type  = rozofs_trc_type_def;
     if (status==0) p->hdr.s.status=0;
     else p->hdr.s.status=1;
     p->hdr.s.index = index;
     p->ino= ino;
     p->errno_val = errno;
     if (fid != NULL) 
     {
        memcpy(p->par.def.fid,fid,sizeof(fid_t)); 
	p->hdr.s.fid = 1;
     }   
     rozofs_trc_wr_idx++;
     if (rozofs_trc_wr_idx >= rozofs_trc_last_idx) 
     {
       rozofs_trc_wr_idx= 0;
       rozofs_trc_buf_full = 1;
     }
   }
}
#else
void rozofs_trc_rsp(int service,fuse_ino_t ino,fid_t fid,int status,int index);

#endif
/*
**____________________________________________________
*/

/**
*  write array statistics counter
*  size in bytes
*/
#define ROZOFS_WRITE_STATS_ARRAY(size) \
{\
   rozofs_write_buf_section_table[(size-1)/ROZOFS_PAGE_SZ]++;\
}

/**
*  read array statistics counter
*  size in bytes
*/
#define ROZOFS_READ_STATS_ARRAY(size) \
{\
   rozofs_read_buf_section_table[(size-1)/ROZOFS_PAGE_SZ]++;\
}

/**
* read/write merge process stats
*/
#define ROZOFS_WRITE_MERGE_STATS(cpt) \
{ \
  rozofs_write_merge_stats_tab[cpt]+=1; \
}

static inline void rozofs_fuse_dbg_save_ctx(rozofs_fuse_save_ctx_t *p)
{
   rozofs_fuse_usr_ctx_idx = (rozofs_fuse_usr_ctx_idx+1)%ROZOFS_FUSE_CTX_MAX;
   rozofs_fuse_usr_ctx_table[rozofs_fuse_usr_ctx_idx] = p;
   
}
/**
**____________________________________________________
*  Get the number of free context in the fuse context distributor

  @param none
  @retval <>NULL, success->pointer to the allocated context
  @retval NULL, error ->out of context
*/
static inline int rozofs_fuse_get_free_ctx_number(void)
{
  return ruc_buf_getFreeBufferCount(rozofs_fuse_ctx_p->fuseReqPoolRef);
}
/*
**__________________________________________________________________________
*/
/**
*  allocate a fuse save context

  @param none
  @retval <>NULL, success->pointer to the allocated context
  @retval NULL, error ->out of context
*/
static inline void *_rozofs_fuse_alloc_saved_context(char *name )
{
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
  void *buffer_p;
//  info("rozofs_fuse_alloc_saved_context %d \n",line);
  
  buffer_p = ruc_buf_getBuffer(rozofs_fuse_ctx_p->fuseReqPoolRef);
  if (buffer_p == NULL) 
  {
     /*
     ** that situation mus not occur
     */
     return NULL;
  }
  /*
  ** Get the payload of the buffer since it is that part that contains
  ** the fuse saved context
  */
  
  fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer_p);
  rozofs_fuse_dbg_save_ctx(fuse_save_ctx_p);
  if (name == NULL) fuse_save_ctx_p->fct_name[0]=0;
  else strcpy(fuse_save_ctx_p->fct_name,name);
  /*
  ** clear the fuse context
  */
  fuse_save_ctx_p->buf_ref = buffer_p;
  fuse_save_ctx_p->buf_flush_offset = 0;
  fuse_save_ctx_p->buf_flush_len    = 0;
  
  fuse_save_ctx_p->newname = NULL;
  fuse_save_ctx_p->name    = NULL;
  fuse_save_ctx_p->fi      = NULL;
  fuse_save_ctx_p->flock   = NULL;
  fuse_save_ctx_p->stbuf   = NULL;
  fuse_save_ctx_p->shared_buf_ref  = NULL;
  /*
  ** init of the routing context
  */
  expgw_routing_ctx_init(&fuse_save_ctx_p->expgw_routing_ctx);
  ruc_listEltInit(&fuse_save_ctx_p->link);
  
  return buffer_p;
}
#define rozofs_fuse_alloc_saved_context() \
  _rozofs_fuse_alloc_saved_context(NULL);
  
/*
**__________________________________________________________________________
*/
/**
*  Release a fuse save context

  @param buffer_p: pointer to the head for the fuse save context (as returned by alloc function)
  @retval none
*/
static inline void _rozofs_fuse_release_saved_context(void *buffer_p,int line)
{
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
//  info("_rozofs_fuse_release_saved_context %d addr %p",line,buffer_p);
  /*
  ** Get the payload of the buffer since it is that part that contains
  ** the fuse saved context
  */
  fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer_p);
  
  fuse_save_ctx_p->fct_name[0] = 0;
  ruc_objRemove(&fuse_save_ctx_p->link);
  /*
  ** clear the fuse context: release strings if any
  */
  
  if (fuse_save_ctx_p->newname != NULL) free(fuse_save_ctx_p->newname);
  if (fuse_save_ctx_p->name != NULL) free((void*)fuse_save_ctx_p->name);
  if (fuse_save_ctx_p->fi!= NULL) free(fuse_save_ctx_p->fi);
  if (fuse_save_ctx_p->flock!= NULL) free(fuse_save_ctx_p->flock);
  if (fuse_save_ctx_p->stbuf!= NULL) free(fuse_save_ctx_p->stbuf);
  if (fuse_save_ctx_p->shared_buf_ref!= NULL) 
  {
    uint32_t *p32 = (uint32_t*)ruc_buf_getPayload(fuse_save_ctx_p->shared_buf_ref);    
    /*
    ** clear the timestamp
    */
    *p32 = 0;
    ruc_buf_freeBuffer(fuse_save_ctx_p->shared_buf_ref);
  }

  /*
  ** check if there is an xmit buffer to release since it might be the case
  ** when there were 2 available load balancing groups
  */
  expgw_routing_release_buffer(&fuse_save_ctx_p->expgw_routing_ctx);
  
  /*
  ** now release the buffer
  */
  ruc_buf_freeBuffer(buffer_p);

}

#define rozofs_fuse_release_saved_context(buf) \
  _rozofs_fuse_release_saved_context(buf,(int)__LINE__);
/*
**__________________________________________________________________________
*/
/**
* API to insert a  fuse request in the read pending list
  @param buffer : pointer to buffer mgt part
  @param f: file_t structure where to queue the buffer
*/
static inline void  fuse_ctx_read_pending_queue_insert(file_t *f,void *buffer_p)
{
     rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
     
     fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer_p);

     ruc_objInsertTail((ruc_obj_desc_t*)&f->pending_rd_list,(ruc_obj_desc_t*)&fuse_save_ctx_p->link);
}


/*
**__________________________________________________________________________
*/
/**
* API to get a  fuse request in the read pending list
  @param f: file_t structure where to queue the buffer
  
    @retval buffer : pointer to buffer mgt part or NULL

*/
static inline void  *fuse_ctx_read_pending_queue_get(file_t *f)
{
     rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
     
     fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*) ruc_objGetFirst((ruc_obj_desc_t*)&f->pending_rd_list);
     if ( fuse_save_ctx_p != NULL)
     {
       ruc_objRemove((ruc_obj_desc_t*)&fuse_save_ctx_p->link);
       return(fuse_save_ctx_p->buf_ref);     
     }
     return NULL;
     
}



/*
**__________________________________________________________________________
*/
/**
* API to insert a  fuse request in the read pending list
  @param buffer : pointer to buffer mgt part
  @param f: file_t structure where to queue the buffer
*/
static inline void  fuse_ctx_write_pending_queue_insert(file_t *f,void *buffer_p)
{
     rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
     
     fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer_p);

     ruc_objInsertTail((ruc_obj_desc_t*)&f->pending_wr_list,(ruc_obj_desc_t*)&fuse_save_ctx_p->link);
}


/*
**__________________________________________________________________________
*/
/**
* API to get a  fuse request in the read pending list
  @param f: file_t structure where to queue the buffer
  
    @retval buffer : pointer to buffer mgt part or NULL

*/
static inline void  *fuse_ctx_write_pending_queue_get(file_t *f)
{
     rozofs_fuse_save_ctx_t *fuse_save_ctx_p;
     
     fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*) ruc_objGetFirst((ruc_obj_desc_t*)&f->pending_wr_list);
     if ( fuse_save_ctx_p != NULL)
     {
       ruc_objRemove((ruc_obj_desc_t*)&fuse_save_ctx_p->link);
       return(fuse_save_ctx_p->buf_ref);     
     }
     return NULL;
     
}
/*
**__________________________________________________________________________
*/
/**
* Macro to save one parameter pointer, uint8_t,uint16_t,uint32_t,uint64_t
* The input arguments:
  @buffer : pointer to the head of the save array context
  @param : field to save
*/
#define SAVE_FUSE_PARAM(buffer,param) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  fuse_save_ctx_p->param = param; \
}

/*
**__________________________________________________________________________
*/
/**
* Macro to save the reception callback called upon end of transaction
* The input arguments:
  @param buffer : pointer to the head of the save array context
  @param : callback-> callback function
*/
#define SAVE_FUSE_CALLBACK(buffer,callback) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer_p); \
  fuse_save_ctx_p->proc_end_tx_cbk= (fuse_end_tx_recv_pf_t)callback; \
}

/*
**__________________________________________________________________________
*/
/**
* Macro to save the reception callback called upon end of transaction
* The input arguments:
  @param buffer : pointer to the head of the save array context
  @param : callback-> callback function
*/
#define GET_FUSE_CALLBACK(buffer,callback) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  callback = fuse_save_ctx_p->proc_end_tx_cbk; \
}
/*
**__________________________________________________________________________
*/
/**
* Macro to save an ascii string in the context
* The input arguments:
  @buffer : pointer to the head of the save array context
  @string_ptr : pointer to the string
*/
#define SAVE_FUSE_STRING(buffer,string_ptr) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  int len = strlen(string_ptr);\
  fuse_save_ctx_p->string_ptr = malloc(len+1); \
  memcpy(fuse_save_ctx_p->string_ptr,string_ptr,len+1);\
}

/*
**__________________________________________________________________________
*/
/**
* Macro to save a structure in the fuse saved context
* The input arguments:
  @buffer : pointer to the head of the save array context
  @str_ptr : pointer to the structure
  @str_len :length of the structure
*/
#define SAVE_FUSE_STRUCT(buffer,str_ptr,str_len) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  if (str_ptr != NULL) \
  { \
    fuse_save_ctx_p->str_ptr = malloc(str_len); \
    memcpy(fuse_save_ctx_p->str_ptr,str_ptr,str_len);\
  }\
  else fuse_save_ctx_p->str_ptr = NULL; \
}

#define RESTORE_FUSE_STRUCT(buffer,str_ptr,str_len) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  memcpy(str_ptr,fuse_save_ctx_p->str_ptr,str_len);\
}


#define RESTORE_FUSE_STRUCT_PTR(buffer,str_ptr) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  str_ptr = fuse_save_ctx_p->str_ptr;\
}


/*
**__________________________________________________________________________
*/
/**
* Macro to save one parameter pointer, uint8_t,uint16_t,uint32_t,uint64_t
* The input arguments:
  @buffer : pointer to the head of the save array context
  @param : field to save
*/
#define RESTORE_FUSE_PARAM(buffer,param) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  param = fuse_save_ctx_p->param ; \
}

#define GET_FUSE_CTX_P(fuse_ctx_p,buffer) \
{ \
  fuse_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
}



#define GET_FUSE_DB(buffer,db) \
{ \
  rozofs_fuse_save_ctx_t *fuse_save_ctx_p = (rozofs_fuse_save_ctx_t*)ruc_buf_getPayload(buffer); \
  db = &fuse_save_ctx_p->db ; \
}

/**
*  Macro METADATA start non blocking case
*/
#define START_PROFILING_NB(buffer,the_probe)\
{ \
  unsigned long long time;\
  struct timeval     timeDay;  \
  gprofiler.the_probe[P_COUNT]++;\
  if (buffer != NULL)\
  { \
    gettimeofday(&timeDay,(struct timezone *)0);  \
    time = MICROLONG(timeDay); \
    SAVE_FUSE_PARAM(buffer,time);\
  }\
}

#define START_PROFILING_IO_NB(buffer,the_probe, the_bytes)\
 { \
  unsigned long long time;\
  struct timeval     timeDay;  \
  gprofiler.the_probe[P_COUNT]++;\
  if (buffer != NULL)\
    {\
        gettimeofday(&timeDay,(struct timezone *)0);  \
        time = MICROLONG(timeDay); \
        SAVE_FUSE_PARAM(buffer,time);\
        gprofiler.the_probe[P_BYTES] += the_bytes;\
    }\
}
/**
*  Macro METADATA stop non blocking case
*/
#define STOP_PROFILING_NB(buffer,the_probe)\
{ \
  unsigned long long timeAfter,time;\
  struct timeval     timeDay;  \
  if (buffer != NULL)\
  { \
    gettimeofday(&timeDay,(struct timezone *)0);  \
    timeAfter = MICROLONG(timeDay); \
    RESTORE_FUSE_PARAM(buffer,time);\
    gprofiler.the_probe[P_ELAPSE] += (timeAfter-time); \
  }\
}


/**
* API for creation a transaction towards an exportd

 The reference of the north load balancing is extracted for the client structure
 fuse_ctx_p:
 That API needs the pointer to the current fuse context. That nformation will be
 saved in the transaction context as userParam. It is intended to be used later when
 the client gets the response from the server
 encoding function;
 For making that API generic, the caller is intended to provide the function that
 will encode the message in XDR format. The source message that is encoded is 
 supposed to be pointed by msg2encode_p.
 Since the service is non-blocking, the caller MUST provide the callback function 
 that will be used for decoding the message
 
 
 @param clt        : pointer to the client structure
 @param timeout_sec : transaction timeout
 @param prog       : program
 @param vers       : program version
 @param opcode     : metadata opcode
 @param encode_fct : encoding function
 @msg2encode_p     : pointer to the message to encode
 @param recv_cbk   : receive callback function
 @param fuse_ctx_p : pointer to the fuse context
 
 @retval 0 on success;
 @retval -1 on error,, errno contains the cause
 */

int rozofs_export_send_common(exportclt_t * clt,uint32_t timeout_sec,uint32_t prog,uint32_t vers,
                              int opcode,xdrproc_t encode_fct,void *msg2encode_p,
                              sys_recv_pf_t recv_cbk,void *fuse_ctx_p);


/**
* API for creation a transaction towards an storcli process

 The reference of the north load balancing is extracted for the client structure
 fuse_ctx_p:
 That API needs the pointer to the current fuse context. That nformation will be
 saved in the transaction context as userParam. It is intended to be used later when
 the client gets the response from the server
 encoding function;
 For making that API generic, the caller is intended to provide the function that
 will encode the message in XDR format. The source message that is encoded is 
 supposed to be pointed by msg2encode_p.
 Since the service is non-blocking, the caller MUST provide the callback function 
 that will be used for decoding the message
 

 @param clt        : pointer to the client structure
 @param timeout_sec : transaction timeout
 @param prog       : program
 @param vers       : program version
 @param opcode     : metadata opcode
 @param encode_fct : encoding function
 @msg2encode_p     : pointer to the message to encode
 @param recv_cbk   : receive callback function
 @param fuse_ctx_p : pointer to the fuse context
  @param storcli_idx      : identifier of the storcli
 @param fid: file identifier: needed for the storcli load balancing context
 
 @retval 0 on success;
 @retval -1 on error,, errno contains the cause
 */

int rozofs_storcli_send_common(exportclt_t * clt,uint32_t timeout_sec,uint32_t prog,uint32_t vers,
                              int opcode,xdrproc_t encode_fct,void *msg2encode_p,
                              sys_recv_pf_t recv_cbk,void *fuse_ctx_p,
			                  int storcli_idx,fid_t fid);


/**
* API for creation a transaction towards an exportd

 The reference of the north load balancing is extracted for the client structure
 fuse_ctx_p:
 That API needs the pointer to the current fuse context. That nformation will be
 saved in the transaction context as userParam. It is intended to be used later when
 the client gets the response from the server
 encoding function;
 For making that API generic, the caller is intended to provide the function that
 will encode the message in XDR format. The source message that is encoded is 
 supposed to be pointed by msg2encode_p.
 Since the service is non-blocking, the caller MUST provide the callback function 
 that will be used for decoding the message
 

 @param eid        : export id
 @param fid        : unique file id (directory, regular file, etc...)
 @param prog       : program
 @param vers       : program version
 @param opcode     : metadata opcode
 @param encode_fct : encoding function
 @msg2encode_p     : pointer to the message to encode
 @param recv_cbk   : receive callback function
 @param fuse_buffer_ctx_p : pointer to the fuse context
 
 @retval 0 on success;
 @retval -1 on error,, errno contains the cause
 */
int rozofs_expgateway_send_routing_common(uint32_t eid,fid_t fid,uint32_t prog,uint32_t vers,
                              int opcode,xdrproc_t encode_fct,void *msg2encode_p,
                              sys_recv_pf_t recv_cbk,void *fuse_buffer_ctx_p) ;




/**
* API for creation a transaction towards an exportd

 The reference of the north load balancing is extracted for the client structure
 fuse_ctx_p:
 That API needs the pointer to the current fuse context. That nformation will be
 saved in the transaction context as userParam. It is intended to be used later when
 the client gets the response from the server
 encoding function;
 For making that API generic, the caller is intended to provide the function that
 will encode the message in XDR format. The source message that is encoded is 
 supposed to be pointed by msg2encode_p.
 Since the service is non-blocking, the caller MUST provide the callback function 
 that will be used for decoding the message
 

 @param rozofs_tx_ctx_p        : transaction context
 @param recv_cbk        : callback function (may be NULL)
 @param fuse_buffer_ctx_p       : buffer containing the fuse context
 @param vers       : program version
 
 @retval 0 on success;
 @retval -1 on error,, errno contains the cause
 */
int rozofs_expgateway_resend_routing_common(rozofs_tx_ctx_t *rozofs_tx_ctx_p, sys_recv_pf_t recv_cbk,void *fuse_buffer_ctx_p) ;

/*
**__________________________________________________________________
*/
/**
*  Some request may trigger an internal flush before beeing executed.

   That's the case of a read request while the file buffer contains
   some data that have not yet been saved on disk, but do not contain 
   the data that the read wants. 

   No fuse reply is expected

 @param fi   file info structure where information related to the file can be found (file_t structure)
 
 @retval 0 in case of failure 1 on success
*/

int rozofs_asynchronous_flush(struct fuse_file_info *fi) ;
/*
**__________________________________________________________________________
*/
/**
  Invalidate the linux cache of a given inode
 
*/

void rozofs_fuse_invalidate_inode_cache(fuse_ino_t ino, uint64_t offset, uint64_t len);
#endif
