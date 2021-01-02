#include <code_utils/cv_utils/pnp/linearpnp.h>

cv::LinearPnP::LinearPnP( const std::vector< Eigen::Vector3d >& pts_2,
                          const std::vector< Eigen::Vector3d >& pts_3 )
{
    int index_max = pts_3.size( );

    M.resize( 2 * index_max, 9 );

    for ( int index = 0; index < index_max; index++ )
    {
        // std::cout << pts_3.at( index ).x << " " << pts_3.at( index ).y << " "
        //           << pts_3.at( index ).z << std::endl;
        // std::cout << pts_2.at( index ).x << " " << pts_2.at( index ).y << " "
        //           << pts_2.at( index ).z << std::endl;
        Eigen::MatrixXd mat_tmp( 2, 9 );
        int max_i
        = math_utils::max_index_in_three( pts_2[index].x( ), pts_2[index].y( ), pts_2[index].z( ) );
        switch ( max_i )
        {
            case 0:
                //                std::cout << " x " << std::endl;
                mat_tmp << -pts_3[index].x( ) * pts_2[index].y( ) / pts_2[index].x( ),
                -pts_3[index].y( ) * pts_2[index].y( ) / pts_2[index].x( ),
                -pts_2[index].y( ) / pts_2[index].x( ), pts_3[index].x( ), pts_3[index].y( ), 1,
                0, 0, 0, -pts_3[index].x( ) * pts_2[index].z( ) / pts_2[index].x( ),
                -pts_3[index].y( ) * pts_2[index].z( ) / pts_2[index].x( ),
                -pts_2[index].z( ) / pts_2[index].x( ), 0, 0, 0, pts_3[index].x( ), pts_3[index].y( ), 1;
                break;
            case 1:
                //                std::cout << " y " << std::endl;
                mat_tmp << pts_3[index].x( ), pts_3[index].y( ), 1,
                -pts_3[index].x( ) * pts_2[index].x( ) / pts_2[index].y( ),
                -pts_3[index].y( ) * pts_2[index].x( ) / pts_2[index].y( ),
                -pts_2[index].x( ) / pts_2[index].y( ), 0, 0, 0, 0, 0, 0,
                -pts_3[index].x( ) * pts_2[index].z( ) / pts_2[index].y( ),
                -pts_3[index].y( ) * pts_2[index].z( ) / pts_2[index].y( ),
                -pts_2[index].z( ) / pts_2[index].y( ), pts_3[index].x( ), pts_3[index].y( ), 1;
                break;
            case 2:
                //                std::cout << " z " << std::endl;
                mat_tmp << pts_3[index].x( ), pts_3[index].y( ), 1, 0, 0, 0,
                -pts_3[index].x( ) * pts_2[index].x( ) / pts_2[index].z( ),
                -pts_3[index].y( ) * pts_2[index].x( ) / pts_2[index].z( ),
                -pts_2[index].x( ) / pts_2[index].z( ), 0, 0, 0, pts_3[index].x( ),
                pts_3[index].y( ), 1, -pts_3[index].x( ) * pts_2[index].y( ) / pts_2[index].z( ),
                -pts_3[index].y( ) * pts_2[index].y( ) / pts_2[index].z( ),
                -pts_2[index].y( ) / pts_2[index].z( );
                break;
            default:
                //                std::cout << " default z " << std::endl;
                mat_tmp << pts_3[index].x( ), pts_3[index].y( ), 1, 0, 0, 0,
                -pts_3[index].x( ) * pts_2[index].x( ) / pts_2[index].z( ),
                -pts_3[index].y( ) * pts_2[index].x( ) / pts_2[index].z( ),
                -pts_2[index].x( ) / pts_2[index].z( ), 0, 0, 0, pts_3[index].x( ),
                pts_3[index].y( ), 1, -pts_3[index].x( ) * pts_2[index].y( ) / pts_2[index].z( ),
                -pts_3[index].y( ) * pts_2[index].y( ) / pts_2[index].z( ),
                -pts_2[index].y( ) / pts_2[index].z( );
                break;
        }

        M.block< 2, 9 >( 2 * index, 0 ) = mat_tmp;
    }

    solvePnP( );
}

void
cv::LinearPnP::solvePnP( )
{
    //    std::cout << "Here is the matrix m:" << std::endl << M << std::endl;
    Eigen::JacobiSVD< Eigen::MatrixXd > svd( M, Eigen::ComputeThinV );
    //    std::cout << "Its singular values are:" << std::endl << svd.singularValues( ) <<
    //    std::endl;

    // the last column of matrixV
    Eigen::Matrix3d hat_H;
    hat_H.block< 1, 3 >( 0, 0 ) = svd.matrixV( ).block< 3, 1 >( 0, 8 ).transpose( );
    hat_H.block< 1, 3 >( 1, 0 ) = svd.matrixV( ).block< 3, 1 >( 3, 8 ).transpose( );
    hat_H.block< 1, 3 >( 2, 0 ) = svd.matrixV( ).block< 3, 1 >( 6, 8 ).transpose( );
    //    std::cout << hat_H << std::endl;

    Eigen::Matrix3d RRT = hat_H;

    T( 0 ) = RRT.coeff( 0, 2 );
    T( 1 ) = RRT.coeff( 1, 2 );
    T( 2 ) = RRT.coeff( 2, 2 );
    std::cout << "T " << T.transpose( ) << std::endl;

    Eigen::Vector3d h1 = RRT.block< 3, 1 >( 0, 0 );
    Eigen::Vector3d h2 = RRT.block< 3, 1 >( 0, 1 );
    Eigen::Vector3d h3 = h1.cross( h2 );

    if ( T( 2 ) < 0 )
    {
        h1 = -h1;
        h2 = -h2;
        T  = -T;
    }

    Eigen::Matrix3d hat_H_2;
    hat_H_2 << h1, h2, h3;
    Eigen::JacobiSVD< Eigen::MatrixXd > svd2( hat_H_2, Eigen::ComputeThinU | Eigen::ComputeThinV );

    R = svd2.matrixU( ) * svd2.matrixV( ).transpose( );
    std::cout << "R " << std::endl << R << std::endl;
    T = T / ( h1.norm( ) );
}
