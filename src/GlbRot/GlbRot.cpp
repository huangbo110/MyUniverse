#include "stdafx.h"
#include "GlbRot.h"
#include <math.h>
#include "opencv.hpp"
#include "IDtest.h"
#include "Ktmfc.h"

#define N_CALIB  (12) //标定纬线的条数
float latitude_calib[N_CALIB+1];

float glbVectorNorm(GlbPoint3d Vec)
{
	return sqrt( Vec.m_x * Vec.m_x + Vec.m_y * Vec.m_y + Vec.m_z * Vec.m_z);
}
void glbVectorNormalize(GlbPoint3d Vec, GlbPoint3d &Vec_dst)
{
	float norm = glbVectorNorm(Vec);
	if (norm<0.00001f) norm=0.00001f;

	Vec_dst.m_x = Vec.m_x / norm;
	Vec_dst.m_y = Vec.m_y / norm;
	Vec_dst.m_z = Vec.m_z / norm;
}
float glbDotMul(GlbPoint3d Vec1, GlbPoint3d Vec2)
{
	return Vec1.m_x*Vec2.m_x + Vec1.m_y*Vec2.m_y + Vec1.m_z*Vec2.m_z;
}
void glbCrossMul(GlbPoint3d Vec1, GlbPoint3d Vec2, GlbPoint3d &VecDST)
{
	GlbPoint3d t;
	t.m_x = Vec1.m_y * Vec2.m_z - Vec2.m_y * Vec1.m_z;
	t.m_y = Vec1.m_z * Vec2.m_x - Vec1.m_x * Vec2.m_z;
	t.m_z = Vec1.m_x * Vec2.m_y - Vec2.m_x * Vec1.m_y;

	VecDST.m_x = t.m_x;
	VecDST.m_y = t.m_y;
	VecDST.m_z = t.m_z;
}

void glbPointGeo2PointRect(GlbPointGeo pGeo, GlbPoint3d &pRect)
{
	float latitude = pGeo.m_lat / 180.0f * PI;//纬度
	float longitude = pGeo.m_lng / 180.0f * PI;//经度

	pRect.m_x = cos(longitude)*cos(latitude);
	pRect.m_y = sin(latitude);
	pRect.m_z = -sin(longitude)*cos(latitude);
}

void glbPointRect2PointGeo(GlbPoint3d pRect, GlbPointGeo &pGeo)
{
	if(pRect.m_y > 1)
	{
		pRect.m_y = 1;
	}
	else if(pRect.m_y<-1)
	{
		pRect.m_y = -1;
	}
	float latitude = asin(pRect.m_y);
	float longitude;
	float r_xz = sqrt(pow(pRect.m_x, 2) + pow(pRect.m_z, 2));
	if( r_xz < 0.0001 )
	{
		longitude = 0;
	}
	else
	{
		longitude = acos(pRect.m_x/r_xz);
	}

	if(pRect.m_z>0)
	{
		longitude = -longitude;
	}
	longitude = longitude / PI * 180.0f;
	latitude = latitude / PI * 180.0f;

	pGeo.m_lng = longitude;
	pGeo.m_lat = latitude;
}

void glbGlobePoint2ScreenPoint(GlbPoint3d p1, GlbRotmat r, GlbPoint3d &p2)
{
	GlbPoint3d t;

	t.m_x = p1.m_x *r.r11 + p1.m_y*r.r12 + p1.m_z*r.r13;
	t.m_y = p1.m_x *r.r21 + p1.m_y*r.r22 + p1.m_z*r.r23;
	t.m_z = p1.m_x *r.r31 + p1.m_y*r.r32 + p1.m_z*r.r33;

	p2.m_x = t.m_x;
	p2.m_y = t.m_y;
	p2.m_z = t.m_z;
}

void glbScreenPoint2GlobePoint(GlbPoint3d p1, GlbRotmat r, GlbPoint3d &p2)
{
	CvMat *GlbRotmat = cvCreateMat(3, 3, CV_32FC1);

	GlbRotmat->data.fl[0] = r.r11;
	GlbRotmat->data.fl[1] = r.r12;
	GlbRotmat->data.fl[2] = r.r13;
	GlbRotmat->data.fl[3] = r.r21;
	GlbRotmat->data.fl[4] = r.r22;
	GlbRotmat->data.fl[5] = r.r23;
	GlbRotmat->data.fl[6] = r.r31;
	GlbRotmat->data.fl[7] = r.r32;
	GlbRotmat->data.fl[8] = r.r33;	

	cvInvert( GlbRotmat, GlbRotmat);

	r.r11 = GlbRotmat->data.fl[0] ;
	r.r12 = GlbRotmat->data.fl[1];
	r.r13 = GlbRotmat->data.fl[2];
	r.r21 = GlbRotmat->data.fl[3];
	r.r22 = GlbRotmat->data.fl[4];
	r.r23 = GlbRotmat->data.fl[5];
	r.r31 = GlbRotmat->data.fl[6];
	r.r32 = GlbRotmat->data.fl[7];
	r.r33 = GlbRotmat->data.fl[8];	

	glbGlobePoint2ScreenPoint(p1, r, p2);
	cvReleaseMat(&GlbRotmat);
}


void glbPointRect2PointRound(GlbPoint3d p3, GlbPointRound &p2)
{
	if(p3.m_y > 1)
	{
		p3.m_y = 1;
	}
	else if(p3.m_y<-1)
	{
		p3.m_y = -1;
	}


	float latitude = asin(p3.m_y);
	float R = (PI/2.0f - latitude)/PI; //范围[0,1]
	float r_xz = sqrt(pow(p3.m_x, 2) + pow(p3.m_z, 2));
	if( r_xz < 0.0001 )
	{
		p2.m_x = 0.0f;
		p2.m_y = 0.0f;
		return;
	}	

	R = glbDistortRadius(R);

	p2.m_x = p3.m_x/r_xz*R;
	p2.m_y = -p3.m_z/r_xz*R;
}

void glbPointRound2PointRect(GlbPoint2d p2, GlbPoint3d &p3)
{
	float R = sqrt(p2.m_x * p2.m_x + p2.m_y *p2.m_y);
	float ix = p2.m_x / R;
	float iz = -p2.m_y / R;

	R = glbUnDistortRadius(R);

	float iR = R;//范围[0,1]
	float latitude = PI/2.0f - iR * PI;
	p3.m_y = sin(latitude);

	if(R < 0.0001)
	{
		p3.m_x = 0;
		p3.m_z = 0;
		return;
	}

	p3.m_x = ix * cos(latitude);
	p3.m_z = iz * cos(latitude);

	return;
}

void glbCreateGlbRotmat(GlbRotmat &r)
{
	r.r11 = 1;
	r.r12 = 0;
	r.r13 = 0;
	r.r21 = 0;
	r.r22 = 1;
	r.r23 = 0;
	r.r31 = 0;
	r.r32 = 0;
	r.r33 = 1;
}

void glbEularAngle2Rotmat(GlbEularAngle angle, GlbRotmat &r)
{
    //欧拉角旋转顺序 Y1 - Z2 - Y3
    //参考：http://en.wikipedia.org/wiki/Euler_angles

    float a1 = angle.m_1_Horz/180.0f*PI;
    float a2 = angle.m_2_Vert/180.0f*PI;
    float a3 = angle.m_3_Axis/180.0f*PI;
    float c1 = cos(a1);
    float c2 = cos(a2);
    float c3 = cos(a3);
    float s1 = sin(a1);
    float s2 = sin(a2);
    float s3 = sin(a3);

    r.r11 = c1*c2*c3-s1*s3;
    r.r12 = -c1*s2;
    r.r13 = c3*s1+c1*c2*s3;
    r.r21 = c3*s2;
    r.r22 = c2;
    r.r23 = s2*s3;
    r.r31 = -c1*s3-c2*c3*s1;
    r.r32 = s1*s2;
    r.r33 = c1*c3-c2*s1*s3;
}

void glbRotVector2RotMat(GlbPoint3d angle, GlbRotmat &r)
{
	CvMat * rotVector1 = cvCreateMat(3, 1, CV_32FC1);
	rotVector1->data.fl[0] = angle.m_x;
	rotVector1->data.fl[1] = angle.m_y;
	rotVector1->data.fl[2] = angle.m_z;
	CvMat *GlbRotmat1 = cvCreateMat(3, 3, CV_32FC1);
	cvRodrigues2(rotVector1, GlbRotmat1);

	r.r11 = GlbRotmat1->data.fl[0];
	r.r12 = GlbRotmat1->data.fl[1];
	r.r13 = GlbRotmat1->data.fl[2];
	r.r21 = GlbRotmat1->data.fl[3];
	r.r22 = GlbRotmat1->data.fl[4];
	r.r23 = GlbRotmat1->data.fl[5];
	r.r31 = GlbRotmat1->data.fl[6];
	r.r32 = GlbRotmat1->data.fl[7];
	r.r33 = GlbRotmat1->data.fl[8];

	cvReleaseMat(&rotVector1);//cvReleaseMatHeader?
	cvReleaseMat(&GlbRotmat1);

}

float glbAngleBetweenPoints(GlbPoint3d p1, GlbPoint3d p2)
{
	//从p1转到p2
	if(p1.m_x == p2.m_x && p1.m_y == p2.m_y && p1.m_z == p2.m_z)
	{
		return 0.0f;
	}
	float fglbDotMul = glbDotMul(p1, p2);
	float m_p1 = glbVectorNorm(p1);
	float m_p2 = glbVectorNorm(p2);

	float d = fglbDotMul / m_p1 / m_p2;

	if(d>1)
	{
		d = 1;
	}
	else if(d<-1)
	{
		d=-1;
	}

	float a = fabs(acos( d ) / PI * 180.0f); //旋转角

	return a;
}

void glbMovingPoints2RotMat(GlbPoint3d p1, GlbPoint3d p2, GlbRotmat &r)
{
	//从p1转到p2
	if(p1.m_x == p2.m_x && p1.m_y == p2.m_y && p1.m_z == p2.m_z)
	{
        glbCreateGlbRotmat(r);
		return;
	}

	float a = glbAngleBetweenPoints(p1, p2)/180.0f*PI; //计算旋转角度
	GlbPivot pivot;
	glbPivotBetweenPoints(p1, p2, pivot); //计算旋转轴

    glbAnglePivot2RotMat(pivot, a, r);
}

void glbAnglePivot2RotMat(GlbPoint3d pivot, float angle, GlbRotmat &r)
{
	angle = angle / 180.0f * PI;
	float x = pivot.m_x;
	float y = pivot.m_y;
	float z = pivot.m_z;

	r.r11 = cos(angle) + (1-cos(angle)) * x * x;
	r.r12 = (1-cos(angle))*x*y - sin(angle)*z;
	r.r13 = (1-cos(angle))*x*z + sin(angle)*y;

	r.r21 = (1-cos(angle))*y*x + sin(angle)*z;
	r.r22 = cos(angle)+(1-cos(angle))*y*y;
	r.r23 = (1-cos(angle))*y*z - sin(angle)*x;

	r.r31 = (1-cos(angle))*z*x - sin(angle)*y;
	r.r32 = (1-cos(angle))*z*y + sin(angle)*x;
	r.r33 = cos(angle) + (1-cos(angle))*z*z;
}

void glbPivotingPoint(GlbPoint3d p, GlbPivot pivot, float angle, GlbPoint3d &p_out)
{
	GlbRotmat rot;
	glbAnglePivot2RotMat(pivot, angle, rot);

	glbGlobePoint2ScreenPoint(p, rot, p_out);
}

void glbCreateNormPivot(GlbPoint3d p, GlbPoint3d directPoint, bool bHeadDirect, GlbPivot &pivot_h, GlbPivot &pivot_v)
{
	if(p.m_x==directPoint.m_x && p.m_y==directPoint.m_y && p.m_z==directPoint.m_z)
	{
		directPoint.m_x = 0;
		directPoint.m_y = 1;
		directPoint.m_z = 0;
	}

	if(p.m_y > 0.999999)
	{
		pivot_v.m_x = 1.0f;
		pivot_v.m_y = 0.0f;
		pivot_v.m_z = 0.0f;
		pivot_h.m_x = 0.0f;
		pivot_h.m_y = 0.0f;
		pivot_h.m_z = 1.0f;
	}
	else
	{
		glbCrossMul(p, directPoint, pivot_v);
		glbVectorNormalize(pivot_v, pivot_v);

		if(!bHeadDirect)
		{
			pivot_v.m_x = -pivot_v.m_x;
			pivot_v.m_y = -pivot_v.m_y;
			pivot_v.m_z = -pivot_v.m_z;
		}

		glbCrossMul(pivot_v, p, pivot_h);
		glbVectorNormalize(pivot_h, pivot_h);
	}
}

void glbInitDistort()
{
    if(!glbKtMFC())
    {
        exit(0);
    }

    //读取纬度标定信息
	for(int i=0; i<N_CALIB+1; i++)
	{
		char Name[20],Default[20], Value[20];
		sprintf_s(Name, 20,"latitude%d", i);
		sprintf_s(Default, 20,"%f", i/(float)N_CALIB);
		GetPrivateProfileString("CALIB", Name, Default, Value, 10, ".\\calib.ini");
		sscanf_s(Value,"%f", latitude_calib+i );
		//！！！！！todo:这里需加入判断维度标定值是否合理！！！！！
	}
}

float glbDistortRadius(float Radius)
{
	float ret=1.0f;
	float fnCalib = (float)(N_CALIB);

	for(int i=1; i<=N_CALIB; i++)
	{
		if(Radius <= i/fnCalib)
		{
			ret = (Radius - (i-1)/fnCalib) * (latitude_calib[i] - latitude_calib[i-1])*fnCalib + latitude_calib[i-1];
			break;
		}
	}
	return ret;
}

float glbUnDistortRadius(float Radius)
{
	float ret=1.0f;
	float fnCalib = (float)(N_CALIB);

	for(int i=1; i<=N_CALIB; i++)
	{
		if(Radius <= latitude_calib[i])
		{
			ret = (Radius - latitude_calib[i-1])/(latitude_calib[i] - latitude_calib[i-1]) / fnCalib + (i-1)/fnCalib;
			break;
		}
	}
	return ret;
}

void glbCloneGlbRotmat( GlbRotmat r, GlbRotmat &r_dst )
{
	r_dst.r11 = r.r11;
	r_dst.r12 = r.r12;
	r_dst.r13 = r.r13;

	r_dst.r21 = r.r21;
	r_dst.r22 = r.r22;
	r_dst.r23 = r.r23;

	r_dst.r31 = r.r31;
	r_dst.r32 = r.r32;
	r_dst.r33 = r.r33;

}

void glbPivotBetweenPoints( GlbPoint3d p1, GlbPoint3d p2, GlbPivot &pivot)
{
	GlbPoint3d axle;//旋转轴（p1叉乘p2）
	glbCrossMul(p1, p2, axle);

	float m_axle = 1;
	m_axle = glbVectorNorm(axle);
	
	//旋转轴单位向量
	pivot.m_x = axle.m_x / m_axle;
	pivot.m_y = axle.m_y / m_axle;
	pivot.m_z = axle.m_z / m_axle;
}

void glbRotmatMul( GlbRotmat mat1, GlbRotmat mat2, GlbRotmat &mat_dst )
{
	GlbRotmat t;
	t.r11 = mat1.r11 * mat2.r11 + mat1.r12 * mat2.r21 + mat1.r13 * mat2.r31;
	t.r12 = mat1.r11 * mat2.r12 + mat1.r12 * mat2.r22 + mat1.r13 * mat2.r32;
	t.r13 = mat1.r11 * mat2.r13 + mat1.r12 * mat2.r23 + mat1.r13 * mat2.r33;

	t.r21 = mat1.r21 * mat2.r11 + mat1.r22 * mat2.r21 + mat1.r23 * mat2.r31;
	t.r22 = mat1.r21 * mat2.r12 + mat1.r22 * mat2.r22 + mat1.r23 * mat2.r32;
	t.r23 = mat1.r21 * mat2.r13 + mat1.r22 * mat2.r23 + mat1.r23 * mat2.r33;

	t.r31 = mat1.r31 * mat2.r11 + mat1.r32 * mat2.r21 + mat1.r33 * mat2.r31;
	t.r32 = mat1.r31 * mat2.r12 + mat1.r32 * mat2.r22 + mat1.r33 * mat2.r32;
	t.r33 = mat1.r31 * mat2.r13 + mat1.r32 * mat2.r23 + mat1.r33 * mat2.r33;

	glbCloneGlbRotmat(t, mat_dst);
}

//int GetTopLayer(GlbPoint2d pTouch)
//{
//	float Radius = g_nFullHeight/2.0;
//	int win_x = g_nFullWidth/2.0 + Radius * pTouch.m_x;
//	int win_y = Radius + Radius * pTouch.m_y;
//
//	GLfloat zbuf;
//	//glReadBuffer(GL_DEPTH_ATTACHMENT_EXT);
//	glReadPixels(win_x, win_y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &zbuf);
//	//printf_s( "win_x, win_y, zbuf: %d, %d, %f.\n", win_x, win_y, zbuf);
//
//	GLint		viewport[4];
//	GLdouble	mvmatrix[16];
//	GLdouble	projmatrix[16];
//	GLdouble	obj_x, obj_y, obj_z;
//	glGetIntegerv(GL_VIEWPORT, viewport);
//	glGetDoublev(GL_MODELVIEW_MATRIX, mvmatrix);
//	glGetDoublev(GL_PROJECTION_MATRIX, projmatrix);
//	gluUnProject( win_x, win_y, zbuf, mvmatrix, projmatrix, viewport, &obj_x, &obj_y, &obj_z);
//	//printf_s( "Object Coordinate: %f, %f, %f.\n", obj_x, obj_y, obj_z );
//	return (int)(obj_z+0.5);
//}