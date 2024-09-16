 //
// matrix.h - homogenous matrix class
// (c) Christian Balkenius 2024-09-13
//

    #include "matrix.h"

namespace ikaros 
{

/*

    h_matrix & reset();
    h_matrix & eye();
    h_matrix & set_rotation_matrix(axis a, float alpha);
    h_matrix & set_translation_matrix(float tx, float ty, float tz);
    h_matrix & set_reflection_matrix(axis a);
    h_matrix & set_scaling_matrix(float sx, float sy, float sz);

    void        get_translation(const h_matrix m, float & x, float & y, float &z);
    void        get_euler_angles(const h_matrix m, float & x, float & y, float &z);
    float       get_euler_angle(const h_matrix m, axis a);
	void 		get_scale(const h_matrix m, float &x, float &y, float &z);
    
    float get_x();
    float get_y();
    float get_z();

*/


    enum axis { X, Y, Z };

    class h_matrix : public matrix
    {
        h_matrix() : matrix(4,4) {};

        h_matrix & reset() { return reset(); }

        h_matrix & 
        eye()
        {
            float * r = data();

            r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
            r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = 0;
            r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = 0;
            r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;

            return *this;
        }

        h_matrix & 
        set_rotation_matrix(axis a, float alpha)
        {
            float * r = data();

            float s = sin(alpha);
            float c = cos(alpha);
            
            switch(a)
            {
                case X:
                    r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
                    r[ 4] = 0; r[ 5] = c; r[ 6] =-s; r[ 7] = 0;
                    r[ 8] = 0; r[ 9] = s; r[10] = c; r[11] = 0;
                    r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                    break;
                
                case Y:
                    r[ 0] = c; r[ 1] = 0; r[ 2] = s; r[ 3] = 0;
                    r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = 0;
                    r[ 8] =-s; r[ 9] = 0; r[10] = c; r[11] = 0;
                    r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                    break;
                
                case Z:
                    r[ 0] = c; r[ 1] = -s; r[ 2] = 0; r[ 3] = 0;
                    r[ 4] = s; r[ 5] = c; r[ 6] = 0; r[ 7] = 0;
                    r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = 0;
                    r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;
                    break;
            }
                
            return *this;
        }


        h_matrix &
        multiply( h_matrix ma, h_matrix mb)
        {
            h_matrix mt;

            float * r = data();
            float * a = ma. data();
            float * b = mb.data();
            float * t = mt.data();
            
            t[0]  = b[0]*a[0]  + b[4]*a[1]  + b[8]*a[2]  + b[12]*a[3];
            t[1]  = b[1]*a[0]  + b[5]*a[1]  + b[9]*a[2]  + b[13]*a[3];
            t[2]  = b[2]*a[0]  + b[6]*a[1]  + b[10]*a[2]  + b[14]*a[3];
            t[3]  = b[3]*a[0]  + b[7]*a[1]  + b[11]*a[2]  + b[15]*a[3];

            t[4]  = b[0]*a[4]  + b[4]*a[5]  + b[8]*a[6]  + b[12]*a[7];
            t[5]  = b[1]*a[4]  + b[5]*a[5]  + b[9]*a[6]  + b[13]*a[7];
            t[6]  = b[2]*a[4]  + b[6]*a[5]  + b[10]*a[6]  + b[14]*a[7];
            t[7]  = b[3]*a[4]  + b[7]*a[5]  + b[11]*a[6]  + b[15]*a[7];

            t[8]  = b[0]*a[8]  + b[4]*a[9]  + b[8]*a[10] + b[12]*a[11];
            t[9]  = b[1]*a[8]  + b[5]*a[9]  + b[9]*a[10] + b[13]*a[11];
            t[10] = b[2]*a[8]  + b[6]*a[9]  + b[10]*a[10] + b[14]*a[11];
            t[11] = b[3]*a[8]  + b[7]*a[9]  + b[11]*a[10] + b[15]*a[11];

            // The following lines could be optimized away if it really is
            // a homogenous matrix. Should always be [0, 0, 0, 1]

            t[12] = b[0]*a[12] + b[4]*a[13] + b[8]*a[14] + b[12]*a[15];
            t[13] = b[1]*a[12] + b[5]*a[13] + b[9]*a[14] + b[13]*a[15];
            t[14] = b[2]*a[12] + b[6]*a[13] + b[10]*a[14] + b[14]*a[15];
            t[15] = b[3]*a[12] + b[7]*a[13] + b[11]*a[14] + b[15]*a[15];
            
            //return copy_array(r, t, 16);
            // memcpy(r, t, 16 * sizeof(float));

            data_ = mt.data_; // FIXME: verify that this works correctly

            return *this;
        }


        h_matrix &
        multiply( h_matrix m)
        {
            multiply(*this, m);
            return *this;
        }



        h_matrix & 
        set_rotation_matrix(float x, float y, float z)
        {
            float * r = data();
            
            h_matrix rX,rY,rZ;

            rX.set_rotation_matrix(X, x);
            rY.set_rotation_matrix(Y, y);
            rZ.set_rotation_matrix(Z, z);

            // memcpy(data(), rZ.data(), 16 * sizeof(float)); //  copy(rZ);
            data_ =  rZ.data_;

            multiply(rY);
            multiply(rX);

            return *this;
        }


        h_matrix & 
        set_translation_matrix(float tx, float ty, float tz)
        {
            float * r = data();

            r[ 0] = 1; r[ 1] = 0; r[ 2] = 0; r[ 3] = tx;
            r[ 4] = 0; r[ 5] = 1; r[ 6] = 0; r[ 7] = ty;
            r[ 8] = 0; r[ 9] = 0; r[10] = 1; r[11] = tz;
            r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;

            return *this; 
        }


        h_matrix & set_reflection_matrix(axis a)
        {
            float * r = data();

            float x = (a == X ? -1 : 1);
            float y = (a == Y ? -1 : 1);
            float z = (a == Z ? -1 : 1);
            r[ 0] = x; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
            r[ 4] = 0; r[ 5] = y; r[ 6] = 0; r[ 7] = 0;
            r[ 8] = 0; r[ 9] = 0; r[10] = z; r[11] = 0;
            r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;

            return *this;
        }


        h_matrix & set_scaling_matrix(float sx, float sy, float sz)
        {
            float * r = data();

            r[ 0] =sx; r[ 1] = 0; r[ 2] = 0; r[ 3] = 0;
            r[ 4] = 0; r[ 5] =sy; r[ 6] = 0; r[ 7] = 0;
            r[ 8] = 0; r[ 9] = 0; r[10] =sz; r[11] = 0;
            r[12] = 0; r[13] = 0; r[14] = 0; r[15] = 1;

            return *this;
        }

        void
        get_translation(float & x, float & y, float &z) 
        {
            float * r = data();




        }


        void
        get_euler_angles(float & x, float & y, float &z)
        {
            float * m = data();



        }


        float
        get_euler_angle( axis a) 
        {
            float * m = data();


            return 0; //******** */
        }


        void 
        get_scale(float &x, float &y, float &z)
        {
            float * m = data();



        }

        float get_x() const { return (*data_)[3]; }
        float get_y() const  { return (*data_)[7]; }
        float get_z() const { return (*data_)[11]; }

    };
}