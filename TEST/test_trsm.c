/*
   PLAPACK Release 3.0
   
   Copyright (C) 2000
   Robert A. van de Geijn and The University of Texas at Austin

   For GNU-license details see the file GNU_license in the 
   PLAPACK root directory.
*/

#include "mpi.h"
#include "PLA.h"

int main(int argc, char *argv[])
{
  MPI_Comm 
    comm = MPI_COMM_NULL;
  MPI_Datatype
    datatype;
  PLA_Template 
    templ = NULL;
  PLA_Obj  
    A      = NULL, B    = NULL,
    alpha  = NULL;
  int      
    m, n,
    nb_distr, 
    error, parameters, sequential, 
    me, nprocs, 
    itype,
    iside, side,
    iuplo, uplo,
    itransa, transa,
    idiag, diag;
  double 
    time,
    flops;

  MPI_Init(&argc, &argv);

  MPI_Comm_rank(MPI_COMM_WORLD, &me);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

  if (me==0) {
    printf("enter distr. block size:\n");
    scanf("%d", &nb_distr );
    printf("nb_distr = %d\n", nb_distr );
    printf("turn on error checking? (0 = NO, 1 = YES):\n");
    scanf("%d", &error );
    printf("error checking = %d\n", error );
    printf("turn on parameter checking? (0 = NO, 1 = YES):\n");
    scanf("%d", &parameters );
    printf("parameter checking = %d\n", parameters );
    printf("turn on sequential checking? (0 = NO, 1 = YES):\n");
    scanf("%d", &sequential );
    printf("sequential checking = %d\n", sequential );
  }
  MPI_Bcast(&nb_distr, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&error, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&parameters, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&sequential, 1, MPI_INT, 0, MPI_COMM_WORLD);

  PLA_Set_error_checking( error, parameters, sequential, FALSE );

  PLA_Comm_1D_to_2D_ratio(MPI_COMM_WORLD, 1.0, &comm);
  PLA_Init( comm );
    
  PLA_Temp_create( nb_distr, 0, &templ );
    
  while ( TRUE ){
    if (me==0) {
      printf("enter datatype:\n");
      printf("-1 = quit\n");
      printf(" 0 = float\n");
      printf(" 1 = double\n");
      printf(" 2 = complex\n");
      printf(" 3 = double complex\n");
      scanf("%d", &itype );
      printf("itype = %d\n", itype );
    }
    MPI_Bcast(&itype, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if ( itype == -1 ) break;
    switch( itype ){
    case 0:
      datatype = MPI_FLOAT;
      break;
    case 1:
      datatype = MPI_DOUBLE;
      break;
    case 2:
      datatype = MPI_COMPLEX;
      break;
    case 3:
      datatype = MPI_DOUBLE_COMPLEX;
      break;
    default:
      PLA_Abort( "unknown datatype", __LINE__, __FILE__ );
    }

    if (me==0) {
      printf("enter m, n:\n");
      scanf("%d %d", &m, &n );
      printf("m = %d, n = %d\n", m , n );
      printf("enter side (0=left, 1=right):\n");
      scanf("%d", &iside );
      printf("iside = %d\n", iside );
      printf("enter uplo (0=lower, 1=upper):\n");
      scanf("%d", &iuplo );
      printf("iuplo = %d\n", iuplo );
      printf("enter transa(0=notrans, 1=trans, 2=conj trans):\n");
      scanf("%d", &itransa );
      printf("itransa = %d\n", itransa );
      printf("enter diag (0=nonunit, 1=unit):\n");
      scanf("%d", &idiag );
      printf("idiag = %d\n", idiag );
    }

    MPI_Bcast(&m,        1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&n,        1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&iside,    1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&iuplo,    1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&itransa,  1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&idiag,    1, MPI_INT, 0, MPI_COMM_WORLD);

    switch( iside ){
    case 0: 
      side = PLA_SIDE_LEFT;
      break;
    case 1: 
      side = PLA_SIDE_RIGHT;
      break;
    }

    switch( iuplo ){
    case 0: 
      uplo = PLA_LOWER_TRIANGULAR;
      break;
    case 1: 
      uplo = PLA_UPPER_TRIANGULAR;
      break;
    }

    switch( itransa ){
    case 0: 
      transa = PLA_NO_TRANS;
      break;
    case 1: 
      transa = PLA_TRANS;
      break;
    case 2: 
      transa = PLA_CONJ_TRANS;
      break;
    default:
      PLA_Abort( "transpose case not supported", __LINE__, __FILE__ );
    }

    switch( idiag ){
    case 0: 
      diag = PLA_NONUNIT_DIAG;
      break;
    case 1: 
      diag = PLA_UNIT_DIAG;
      break;
    }

    PLA_Matrix_create( datatype, 
		       ( side == PLA_SIDE_LEFT ? m : n ),
		       ( side == PLA_SIDE_LEFT ? m : n ),
			templ, 
                        PLA_ALIGN_FIRST, 
                        PLA_ALIGN_FIRST, 
                        &A ); 

    create_problem( A );

    {
      double d_n;

      d_n = (double) n;
      PLA_Shift( A, MPI_DOUBLE, &d_n );
    }

    PLA_Matrix_create( datatype, 
		        m, n,
		        templ, 
                        PLA_ALIGN_FIRST, 
                        PLA_ALIGN_FIRST, 
                        &B ); 
      
    create_problem( B );

    PLA_Mscalar_create( datatype, 
		        PLA_ALL_ROWS,
                        PLA_ALL_COLS,
			1, 
			1,
			templ, 
                        &alpha ); 

    PLA_Obj_set_to_one( alpha );

    MPI_Barrier( MPI_COMM_WORLD );
    time = MPI_Wtime ();

    PLA_Trsm( side, uplo, transa, diag, alpha, A, B );

    MPI_Barrier( MPI_COMM_WORLD );
    time = MPI_Wtime () - time;

    if ( side == PLA_SIDE_LEFT ) 
      flops = 1.0 * m * m * n;
    else
      flops = 1.0 * m * n * n;

    if ( datatype == MPI_COMPLEX || datatype == MPI_DOUBLE_COMPLEX )
      flops *= 4;

    if ( me == 0 ) 
      printf("%d %d time = %f, MFLOPS/node = %10.4lf \n", m, n, time,
	       flops / time * 1.0e-6 / nprocs );
  }

  PLA_Obj_free( &A );
  PLA_Obj_free( &B );
  PLA_Obj_free(&alpha);

  PLA_Temp_free(&templ);
  PLA_Finalize( );
  MPI_Finalize( );
  
}
