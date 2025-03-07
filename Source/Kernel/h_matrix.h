 //
// matrix.h - homogenous matrix class
// (c) Christian Balkenius 2024-09-13
//

#pragma once

    #include "maths.h"
    #include "matrix.h"


namespace ikaros 
{
    enum axis { X, Y, Z };

    class h_matrix : public matrix
    {
    public:

        //h_matrix() : matrix(4,4) {};
        h_matrix() : matrix(4,4) {eye();}; 

        h_matrix & reset() { return reset(); }
        
        h_matrix & copy(h_matrix m) 
        {
        
            matrix BaseMatrix;
            matrix HMatrix_BaseMatrix;
            HMatrix_BaseMatrix = m;
            BaseMatrix.copy(HMatrix_BaseMatrix);
            copy(BaseMatrix);
            return *this;
        }

    
        h_matrix & copy(matrix mat) 
        {
            matrix *p = this;
            p->copy(mat);
          return *this;
        }


        bool is_valid() { 
            float * r = data(); 
            return (r[15] > 0.1); 
            } // Matrix is valid if bottom right element is 1 (but we do not want to make an exact match)

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

            *data_ = *(mt.data_);

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
            *data_ =  *(rZ.data_);

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


        h_matrix & 
        set_reflection_matrix(axis a)
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


        h_matrix & 
        set_scaling_matrix(float sx, float sy, float sz)
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
            float * m = data();

        x = m[3];
        y = m[7];
        z = m[11];
        }


        void
        get_euler_angles(float & x, float & y, float &z)
        {
            float * m = data();

            if (m[8] < +1)
            {
                if (m[8] > -1)
                {
                    y = asin(-m[8]);
                    z = ::atan2(m[4],m[0]);
                    x = ::atan2(m[9],m[10]);
                }
                else // m8 = -1
                {
                    y = +pi/2;
                    z = -::atan2(-m[6],m[5]);
                    x = 0;
                }
            }
            else // m8 = +1
            {
                y = -pi/2;
                z = ::atan2(-m[6],m[5]);
                x = 0;
            }
        }


        float
        get_euler_angle( axis a) 
        {
            float * m = data();

              switch(a)
        {
            case X:
                if (-1 < m[8] && m[8] < +1)
                    return ::atan2(m[9], m[10]);
                else
                    return  0;
            
            case Y:
                if(m[8] >= +1)
                    return  -pi/2;
                else if (m[8] > -1)
                    return asin(-m[8]);
                else
                    return +pi/2;

            case Z:
                if(m[8] >= +1)
                    return  ::atan2(-m[6], m[5]);
                else if (m[8] > -1)
                    return ::atan2(m[4], m[0]);
                else
                    return -::atan2(-m[6], m[5]);
        }
    
        return 0;
        }


        void 
        get_scale(float &x, float &y, float &z)
        {
            float * m = data();

            x = m[0];
            y = m[5];
            z = m[10];
        }

        float get_x() const { return (*data_)[3]; }
        float get_y() const  { return (*data_)[7]; }
        float get_z() const { return (*data_)[11]; }


        h_matrix &
        multiply_v(h_matrix mm, matrix vv) // FIXME: Should check size of vv, should be matrix(4) ****************
    {
        float * r = data();
        float * m = mm.data();
        float * v = vv.data();

        float t[4];
        t[0] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3] * v[3];
        t[1] = m[4] * v[0] + m[5] * v[1] + m[6] * v[2] + m[7] * v[3];
        t[2] = m[8] * v[0] + m[9] * v[1] + m[10] * v[2] + m[11] * v[3];
        t[3] = m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3];

        if(t[3] != /* DISABLED CODE */ (0) && false) // normalize vector
        {
            r[0] = t[0] / t[3];
            r[1] = t[1] / t[3];
            r[2] = t[2] / t[3];
            r[3] = 1;
        }
        else // should never happen
        {
            r[0] = t[0];
            r[1] = t[1];
            r[2] = t[2];
            r[3] = 1;
        }
        
        return *this;
    }
    
    h_matrix &
    h_transpose(h_matrix aa)
    {
        float t[16];
        float * a = aa.data();

        t[ 0] = a[ 0]; t[ 1] = a[ 4]; t[ 2] = a[ 8]; t[ 3] = a[12];
        t[ 4] = a[ 1]; t[ 5] = a[ 5]; t[ 6] = a[ 9]; t[ 7] = a[13];
        t[ 8] = a[ 2]; t[ 9] = a[ 6]; t[10] = a[10]; t[11] = a[14];
        t[12] = a[ 3]; t[13] = a[ 7]; t[14] = a[11]; t[15] = a[15];

        //return copy_array(r, t, 16);

        std::copy(t, t + 16, data_->begin());

        return *this;
    }
    


    };
}