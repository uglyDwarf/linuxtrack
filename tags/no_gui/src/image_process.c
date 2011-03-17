#include <stdbool.h>
#include "image_process.h"
#include "list.h"
#include "utils.h"

struct my_blob_type {
  /* coordinates of the blob on the screen 
   * these coordinates will have the center of 
   * the screen at (0,0).*
   * (+resx/2,+resy/2) = top right corner
   * (-resx/2,-resy/2) = bottom left corner
   */
  float x,y; 
  /* total # pixels area, used for sorting/scoring blobs */
  unsigned int score;
  float sum; 
};


int get_blob(unsigned char *pt,unsigned char *low_limit,
               unsigned char *high_limit,
	       unsigned char *left_limit,
	       int line,struct my_blob_type *b, int linelen, int rec_level)
{
  unsigned char *p_ul,*p_u,*p_ur,*p_l,*p_r,*p_dl,*p_d,*p_dr;
  if(rec_level>1000){
    log_message("Recursion level reached!\n");
    return(0);
  }
  rec_level++;
  int x = (pt - left_limit);
  ++b->score;
  b->sum += (int)*pt;
  b->x += (float)x * (float)*pt;
  b->y += (float)line * (float)*pt;
  *pt = 0;
  
  p_l=pt-1;
  p_r=pt+1;
  p_u=pt-linelen;
  p_d=pt+linelen;
  
  if(left_limit>p_l) p_l=NULL;
  if(left_limit+linelen<=p_r) p_r=NULL;
  if(low_limit>p_u) p_u=NULL;
  if(high_limit<=p_d) p_d=NULL;
  
  if((p_l!=NULL)&&(p_u!=NULL)) p_ul=p_u-1; else p_ul=NULL;
  if((p_r!=NULL)&&(p_u!=NULL)) p_ur=p_u+1; else p_ur=NULL;
  if((p_l!=NULL)&&(p_d!=NULL)) p_dl=p_d-1; else p_dl=NULL;
  if((p_r!=NULL)&&(p_d!=NULL)) p_dr=p_d+1; else p_dr=NULL;
  
  if((p_ul!=NULL)&&(*p_ul > 0)) get_blob(p_ul,low_limit,high_limit,left_limit-linelen,line-1,b,linelen, rec_level);
  if((p_u!=NULL)&&(*p_u > 0)) get_blob(p_u,low_limit,high_limit,left_limit-linelen,line-1,b,linelen, rec_level);
  if((p_ur!=NULL)&&(*p_ur > 0)) get_blob(p_ur,low_limit,high_limit,left_limit-linelen,line-1,b,linelen, rec_level);

  if((p_l!=NULL)&&(*p_l > 0)) get_blob(p_l,low_limit,high_limit,left_limit,line,b,linelen, rec_level);
  if((p_r!=NULL)&&(*p_r > 0)) get_blob(p_r,low_limit,high_limit,left_limit,line,b,linelen, rec_level);

  if((p_dl!=NULL)&&(*p_dl > 0)) get_blob(p_dl,low_limit,high_limit,left_limit+linelen,line+1,b,linelen, rec_level);
  if((p_d!=NULL)&&(*p_d > 0)) get_blob(p_d,low_limit,high_limit,left_limit+linelen,line+1,b,linelen, rec_level);
  if((p_dr!=NULL)&&(*p_dr > 0)) get_blob(p_dr,low_limit,high_limit,left_limit+linelen,line+1,b,linelen, rec_level);
  return(0);
}



int search_for_blobs(unsigned char *buf, int w, int h,
                     struct bloblist_type *blobs, int min, int max)
{
  int max_blobs = blobs->num_blobs;
  int counter = 0;
  unsigned char *limit=buf+h*w;
  unsigned char *ptr;
  struct my_blob_type blob;
  blobs->blobs = (struct blob_type *)
    my_malloc(max_blobs * sizeof(struct blob_type));
  
  ptr=buf;
  while(ptr<limit){
    if(*ptr > 0){
      int line=(ptr-buf)/w;
      blob.x = blob.y = 0.0f;
      blob.score = 0;
      blob.sum = 0;
      get_blob(ptr,buf,limit,buf+(line*w),line,&blob,w,0);
      if((blob.score > min) && (blob.score < max)){
//        printf("Have blob %f, %f - %d points!\n", blob.x/blob.sum, blob.y/blob.sum,
//               blob.score);
        struct blob_type *tmp = &(blobs->blobs[counter]); 
        tmp->x = (w / 2.0) - (blob.x / blob.sum);
        tmp->y = (h / 2.0) - (blob.y / blob.sum);
        tmp->score = blob.score;
        ++counter;
      }
    }
    if(counter >= max_blobs){
      break;
    }
    ++ptr;
  }
  if(counter == max_blobs){
    return true;
  }
  free(blobs->blobs);
  blobs->num_blobs = 0;
  return false;
}
