#include "rv_declarations.h"

/*** global declarations side 0: ***/
float  rvs0_rv_mult(float  x, float  y);
float  rvs0_rv_div(float  x, float  y);
int  rvs0_rv_mod(int  x, int  y);
void  __CPROVER_assume(_Bool  rv_arg_0);
void  assert(_Bool  rv_arg_1);
void  rvs0_bubble_sort(int  *a, int  sz);
int  rvs0_a[5];
void  rvs0_bubble_sort(int  *a, int  sz);
int  rvs0_init_array(int  n);
int  rvs0_main();
unsigned char  rvs0_Loop_bubble_sort_for1(int  *a, int  sz, int  *rvp_i);
unsigned char  chk0_Loop_bubble_sort_for1_for1(int  *a, int  i, int  *rvp_j);
void  __CPROVER_assume(_Bool  rv_arg_6);

/*** global declarations side 1: ***/
float  rvs1_rv_mult(float  x, float  y);
float  rvs1_rv_div(float  x, float  y);
int  rvs1_rv_mod(int  x, int  y);
void  __CPROVER_assume(_Bool  rv_arg_2);
void  assert(_Bool  rv_arg_3);
void  rvs1_bubble_sort(int  *a, int  sz);
int  rvs1_a[5];
void  rvs1_bubble_sort(int  *a, int  sz);
int  rvs1_init_array(int  n);
int  rvs1_main();
unsigned char  rvs1_Loop_bubble_sort_for1(int  *a, int  sz, int  *rvp_i);
unsigned char  chk1_Loop_bubble_sort_for1_for1(int  *a, int  i, int  *rvp_j);
void  __CPROVER_assume(_Bool  rv_arg_7);

/*** end of global declarations side 1: ***/
/*** end of global declarations. ***/

/*** Functions side 0: ***/
unsigned char  chk0_Loop_bubble_sort_for1_for1(int  *a, int  i, int  *rvp_j)
{
  int  t;

  if (!(*rvp_j < i))
    return 0;
  {
    __CPROVER_assume(*rvp_j < 4 && *rvp_j >= 0);
    if (a[*rvp_j] < a[*rvp_j + 1])
    {
      t = a[*rvp_j];
      a[*rvp_j] = a[*rvp_j + 1];
      a[*rvp_j + 1] = t;
      a[*rvp_j] = a[*rvp_j];
    }

  }

  (*rvp_j)++;
  return 0;
}


/*** Functions side 1: ***/
unsigned char  chk1_Loop_bubble_sort_for1_for1(int  *a, int  i, int  *rvp_j)
{
  int  t;

  if (!(*rvp_j < i))
    return 0;
  {
    __CPROVER_assume(*rvp_j < 4 && *rvp_j >= 0);
    if (*(a + *rvp_j) < *(a + *rvp_j + 1))
    {
      t = *(a + *rvp_j);
      *(a + *rvp_j) = *(a + *rvp_j + 1);
      *(a + *rvp_j + 1) = t;
    }

    *rvp_j = *rvp_j + 1;
  }

  return 0;
}






void rv_init()
{
}


void rv_check()
{
  /* check reach-equivalence of the inputs for each uninterpreted function: */
  int i;
}

  /* now starting main */

int main()
{
  _Bool equal = 1;
  /* Declarations: */
  unsigned char  retval0;	/* Generated by:  gen_retval_declarations/ rv_temps.cpp:269*/
  unsigned char  retval1;	/* Generated by:  gen_retval_declarations/ rv_temps.cpp:269*/
  int  rvs0_a[5];	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  rvs1_a[5];	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  rvs0_i;	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  rvs1_i;	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  *rvs0_rvp_j;	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  *rvs1_rvp_j;	/* Generated by:  gen_arg_decl(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)(107)/ gen_decl_low(148)/ rv_temps.cpp:274*/
  int  rv_D0_0;	/* Generated by:  gen_arg_alloc_side(601)(601)(601)(326)/ gen_safe_alloc/ rv_temps.cpp:327*/
  int  rv_D1_0;	/* Generated by:  gen_arg_alloc_side(601)(601)(601)(326)/ gen_safe_alloc/ rv_temps.cpp:327*/


  /* output: */
  /* output: */
  /* output: */
  /* Allocations for side 0: */
  rvs0_rvp_j = &rv_D0_0;	/* Generated by:  gen_arg_alloc_side(601)(601)(601)(326)/ gen_safe_alloc/ rv_temps.cpp:332*/
  /* Allocations for side 1: */
  rvs1_rvp_j = &rv_D1_0;	/* Generated by:  gen_arg_alloc_side(601)(601)(601)(326)/ gen_safe_alloc/ rv_temps.cpp:332*/

  rv_init();

  /* Assume input args are equal: */ 
  RVT_COPY_ARRAY(rvs0_a,rvs1_a, 5);	/* Generated by:  gen_args_equality(608)(613)(379)(289)/ rv_temps.cpp:368*/
  rvs0_i = rvs1_i;	/* Generated by:  gen_args_equality(608)(613)(379)(289)(608)(613)(379)(293)/ rv_temps.cpp:362*/
  *rvs0_rvp_j = *rvs1_rvp_j;	/* Generated by:  gen_args_equality(608)(613)(379)(289)(608)(613)(379)(293)(608)/ protect_pointer [op=4](613)(379)(293)/ rv_temps.cpp:362*/

  /* run each side's main() */
  retval0 = chk0_Loop_bubble_sort_for1_for1(rvs0_a, rvs0_i, rvs0_rvp_j);
  retval1 = chk1_Loop_bubble_sort_for1_for1(rvs1_a, rvs1_i, rvs1_rvp_j);

  rv_check();

  return 0;
}