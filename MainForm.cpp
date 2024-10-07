#include "MainForm.h"
#include <fstream>
#include <cmath>
#include <algorithm>
#include "MetricCalculation.h"
#include "BitmapConverter.h"
#include "TMO.h"
#include "xvtCV/GammaCorrector.h"
#include "xvtCV/Utils.h"

std::vector<cv::Rect> lstROI;
cv::Rect subROI;
cv::Point movePos;
cv::Point drawPos;
cv::Mat image16bit;
std::wstring fileName;
std::vector<uint8_t> LUT;
Metric metric;
float gamma = 5;

qualitymeasurement::MainForm::MainForm(void)
	: m_isDraw(false)
	, m_isMove(false)
	, m_isMouseDown(false)
	, m_isMoving(false)
	, m_totalImage(0)
	, m_imageIdx (0)
	, m_scale(0)
	, m_rect(System::Drawing::Rectangle(0, 0, 0, 0))
	, m_rectLeft(System::Drawing::Rectangle(0, 0, 0, 0))
	, m_rectRight(System::Drawing::Rectangle(0, 0, 0, 0))
	, m_picBoxOrgSize (System::Drawing::Rectangle(0, 0, 0, 0))
	, m_centerPanel(System::Drawing::Point(0, 0))
	, m_offsetPoint(System::Drawing::Point(0, 0))
{
	InitializeComponent();
	
	//
	//TODO: Add the constructor code here
	//
	m_centerPanel = System::Drawing::Point(panel3->Location.X + panel3->Width/2, panel3->Location.Y + panel3->Height / 2);
	
	this->panel3->MouseWheel += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::pictureBox_MouseWheel);
	//Load Typical Value
	LoadTypicalValue();
	
	this->dataGridView1->Rows->Add("SNR (dB)", typicalValue->SNR, 0, 0, 0);
	this->dataGridView1->Rows->Add("CTF (%)", typicalValue->CTF, 0, 0, 0);
	this->dataGridView1->Rows->Add("CNR", typicalValue->CNR, 0, 0, 0);
}

qualitymeasurement::MainForm::~MainForm()
{
	if (components)
	{
		delete components;
	}
	if (typicalValue)
	{
		delete typicalValue;
	}
}

cv::Rect RoiRefinement(cv::Rect selectROI, cv::Size imageSize)
{
	int d_x = selectROI.x < 0 ? 0 : selectROI.x;
	int d_y = selectROI.y < 0 ? 0 : selectROI.y;
	int d_width = (d_x + selectROI.width <= imageSize.width) ? selectROI.width : (imageSize.width - d_x);
	int d_height = (d_y + selectROI.height <= imageSize.height) ? selectROI.height : (imageSize.height - d_y);
	if (d_x >= imageSize.width) {
		d_x = imageSize.width - 1;
		d_width = 0;
	}
	if (d_y >= imageSize.height) {
		d_y = imageSize.height - 1;
		d_height = 0;
	}

	return cv::Rect(d_x, d_y, d_width, d_height);
}

void qualitymeasurement::MainForm::caculateScaleOffset()
{
	float diffRatio = (float)m_image->Width / panel3->Width - (float)m_image->Height / panel3->Height;
	if (diffRatio > 0)
	{
		m_scale = (float)panel3->Width / m_image->Width;
		m_offsetPoint.X = 0;
		m_offsetPoint.Y = (int)(panel3->Height / 2 - m_image->Height * m_scale / 2);
	}
	else
	{
		m_scale = (float)panel3->Height / m_image->Height;
		m_offsetPoint.X = (int)(panel3->Width / 2 - m_image->Width * m_scale / 2);
		m_offsetPoint.Y = 0;
	}

	pictureBox->Location = m_offsetPoint;
	pictureBox->Width = m_image->Width * m_scale;
	pictureBox->Height = m_image->Height * m_scale;
	m_picBoxOrgSize = System::Drawing::Rectangle(pictureBox->Location.X, pictureBox->Location.Y, pictureBox->Width, pictureBox->Height);
}

System::Void qualitymeasurement::MainForm::pictureBox_MouseDown(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
{
	if (e->Button ==  System::Windows::Forms::MouseButtons::Right && !image16bit.empty())
	{
		rotateFlag ++;
		if (rotateFlag > 3)
		{
			rotateFlag = 0;
		}
		if (rotateFlag < 4)
		{
			cv::rotate(image16bit, image16bit, 0);
		}
		
		if (image16bit.type() == CV_16UC1)
		{
			cv::Mat image8;// = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));
			xvt::Convert8Bits(image16bit, image8);
			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			m_image = bitmap;
		}
		else
		{
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image16bit);
			m_image = bitmap;
		}
		
		Update();
	}
	else
	{
		m_isMouseDown = true;

		int imageXPosition = e->Location.X / m_scale;
		int imageYPosition = e->Location.Y / m_scale;
		strXY->Text = L"(" + imageXPosition + ":" + imageYPosition + ")";
		System::Drawing::Rectangle rect;
		rect .Location = e->Location;

		if (rect.Right > pictureBox->Width)
		{
			rect.X = pictureBox->Width - rect.Width;
		}
		if (rect.Top < 0)
		{
			rect.Y = 0;
		}
		if (rect.Left < 0)
		{
			rect.X = 0;
		}
		if (rect.Bottom > pictureBox->Height)
		{
			rect.Y = pictureBox->Height - rect.Height;
		}

		if (m_isDraw)
		{
			m_rect.Location = rect.Location;
			m_rect.Width = 0;
			m_rect.Height = 0;
			m_rectLeft = System::Drawing::Rectangle(0, 0, 0, 0);
			m_rectRight = System::Drawing::Rectangle(0, 0, 0, 0);
			drawPos = cv::Point(e->Location.X, e->Location.Y);
		}
		
		if(m_isMove)
		{
			array<System::Drawing::Rectangle>^ rectList = gcnew array<System::Drawing::Rectangle>(8);
			//Top Left
			System::Drawing::Rectangle rect (m_rect.X - 4, m_rect.Y - 4, 9, 9);
			rectList[0] = rect;
			//Bottom Right
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			rectList[1] = rect;
			//Bottom Left
			rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			rectList[2] = rect;
			//Top Right
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y - 4, 9, 9);
			rectList[3] = rect;
			//Top
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y - 4, 9, 9);
			rectList[4] = rect;
			//Left
			rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
			rectList[5] = rect;
			//Bottom
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			rectList[6] = rect;
			//Right
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
			rectList[7] = rect;

			for (int i = 0; i < 8; i++)
			{
				if (m_rect.Width > 0 && m_rect.Height > 0)
				{
					if (isMouseInsideRect(rectList[i], e))
					{
						m_isMoving = true;
						movePos = cv::Point(e->Location.X, e->Location.Y);
						m_rectLeft = System::Drawing::Rectangle(0, 0, 0, 0);
						m_rectRight = System::Drawing::Rectangle(0, 0, 0, 0);
						break;
					}
					else if (i == 7)
					if (e->Location.X > m_rect.X && (e->Location.X < m_rect.X + m_rect.Width)
						&& e->Location.Y > m_rect.Y && (e->Location.Y < m_rect.Y + m_rect.Height))
					{
						m_isMoving = true;
						movePos = cv::Point(e->Location.X, e->Location.Y);
						m_rectLeft = System::Drawing::Rectangle(0, 0, 0, 0);
						m_rectRight = System::Drawing::Rectangle(0, 0, 0, 0);
					}
					else
					{
						m_isMoving = false;
						movePos = cv::Point(0,0);
					}
				}
			}
		}
	}
}

Bitmap^ qualitymeasurement::MainForm::GraphDraw(System::Drawing::Graphics^ grap, std::vector<int> xCoordinate, std::vector<double> list, std::string title, std::string lable, double typical)
{
	System::Drawing::Rectangle m_rectBox = System::Drawing::Rectangle(0, 0, m_cum_width, m_cum_height);
	int scale = 2;
	cv::Mat mat(m_cum_height * scale, m_cum_width * scale, CV_8UC3);
	mat.setTo(cv::Scalar(255, 255, 255));
	CvPlot::Axes axes = CvPlot::makePlotAxes();
	//axes.enableHorizontalGrid();
	//axes.enableVerticalGrid();
	//axes.title(title);
	//axes.yLabel(lable);
	if (xCoordinate.size() > 1)
	{
		axes.setXLim(std::pair<int, int>(xCoordinate.front(), xCoordinate.back()));
	}
	else if (xCoordinate.size() == 1)
	{
		axes.setXLim(std::pair<int, int>(0, xCoordinate.back() * 2));
	}
	else
	{
		axes.setXLim(std::pair<int, int>(0, 100));
	}
	axes.create<CvPlot::Series>(xCoordinate, list, "-ob");
	axes.setMargins(35, 15, 40, 25);
	axes.create<CvPlot::Series>(xCoordinate, list, "-ob");
	double xLim = 2000;
	axes.create<CvPlot::Series>(std::vector<cv::Point2d>{{-xLim, typical}, {xLim, typical}}, "-r");
	axes.render(mat);

	cv::Mat resizeImg;
	cv::resize(mat, resizeImg, cv::Size(m_cum_width, m_cum_height));

	//cv::imshow(title, resizeImg);

	Bitmap^ bmp1 = BitmapConverter::ToBitmap(resizeImg);
	mat.release();
	resizeImg.release();
	return bmp1;
}

Bitmap^ qualitymeasurement::MainForm::GraphDraw(System::Drawing::Graphics^ grap, std::vector<double> list, std::string title, std::string lable)
{
	int scale = 2;
	cv::Mat mat(100*scale, 830*scale, CV_8UC3);
	mat.setTo(cv::Scalar(255, 255, 255));
	CvPlot::Axes axes = CvPlot::makePlotAxes();
	axes.enableHorizontalGrid();
	axes.enableVerticalGrid();
	//axes.title(title);
	//axes.yLabel(lable);
	axes.create<CvPlot::Series>(list, "-b");
	axes.setMargins(50, 30, 10, 30);
	axes.setXLim(std::pair<int, int>(0, list.size()));
	axes.setXTight(true);
	double maxList = *max_element(list.begin(), list.end());
	double minList = *min_element(list.begin(), list.end());
	//axes.setYLim(std::pair<int, int>((int)round(minList - 50), (int)round(maxList + 50)));
	axes.render(mat);

	cv::Mat resizeImg;
	cv::resize(mat, resizeImg, cv::Size(830, 100));

	//cv::imshow(title, resizeImg);

	Bitmap^ bmp1 = BitmapConverter::ToBitmap(resizeImg);
	mat.release();
	resizeImg.release();
	return bmp1;
}

void qualitymeasurement::MainForm::Update()
{
	System::Drawing::Pen^ redPen = gcnew System::Drawing::Pen(Color::Red, 2);
	System::Drawing::Graphics^ grapCNRLeft = panel3->CreateGraphics();
	grapCNRLeft->DrawEllipse(redPen, System::Drawing::Rectangle(m_centerPanel.X - 10, m_centerPanel.Y - 10, 20, 20));

	//Write XML file
	if (!typicalValue)
	{
		typicalValue = gcnew TypicalValue;
	}
	typicalValue->SaveToXml(xmlFile);
	
	if (m_totalImage <= 0)
	{
		Reset();
		return;
	}

	btnMove->Text = m_isMove ? L"Draw" : L"Move";

	btnExportResult->Enabled = true;
	btnNext->Enabled = true;
	btnPrevious->Enabled = true;

	strIndex->Text = L"" + (m_imageIdx + 1).ToString() +"/" + m_totalImage.ToString();

	this->pictureBox->Image = m_image;
	double CTFLeft = 0;
	double CTFRight = 0;
	double CTFAvg = 0;
	double SNRLeft = 0;
	double SNRRight = 0;
	double SNRAvg = 0;
	double CNRLeft = 0;
	double CNRRight = 0;
	double CNRAvg = 0;
	m_rectLeft = System::Drawing::Rectangle(0, 0, 0, 0);
	m_rectRight = System::Drawing::Rectangle(0, 0, 0, 0);
	if (subROI.width > 0 && subROI.height > 0)
	{
		btnApplyAll->Enabled = true;
		btnMove->Enabled = true;

		//Phu hot fix
		if (image16bit.type() == CV_16UC1)
		{
			cv::Point2f blackPoint, whitePoint;
			int minIntensityImg, maxIntensityImg;
			calculateLUTPiecewisePoints(image16bit, subROI, blackPoint, whitePoint, minIntensityImg, maxIntensityImg);
			//generateToneMappingLUT(blackPoint, whitePoint, LUT, minIntensityImg, maxIntensityImg);
			cv::Mat image8;// = toneMapping(image16bit, LUT);
			xvt::enhance::GammaCorrector Gamma;
			Gamma.SetGamma(gamma);
			xvt::Convert8Bits(image16bit, image8);
			Gamma.Apply(image8, image8);
			m_image = BitmapConverter::ToBitmap(image8);
			this->pictureBox->Image = m_image;
		}
		else
		{
			/*if (image16bit.type() == CV_16UC1)
				m_image = BitmapConverter::ToBitmap(image16bit);
			this->pictureBox->Image = m_image;*/
			
		}
		
		cv::Mat src = cv::Mat(m_image->Height, m_image->Width, CV_8UC3);
		Bitmap^ bmp = gcnew Bitmap(m_image);
		BitmapConverter::ToMat(bmp, src);
		bmp->~Bitmap();
		cv::Mat dst;
		cvtColor(src, dst, cv::COLOR_RGB2GRAY);
#ifdef _DEBUG
		//cv::namedWindow("subROI Image", cv::WINDOW_NORMAL);
		//cv::imshow("subROI Image", dst(subROI));
#endif // _DEBUG
		//Metric metric;
		//MetricCalculationWholeRegion(dst(subROI), metric);
		if(radioButtonROICertainArea->Checked)
		{
		    MetricCalculation(image16bit(subROI), metric);
		}
		else
		{
		    MetricCalculationWholeRegion(image16bit(subROI), metric);
		}
		
	}
	else
	{
		if (m_rotateUpdate == true)
		{
			cv::Mat image8 = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));

			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			this->pictureBox->Image = m_image;
		}
		btnMove->Enabled = false;
		m_isMove = false;
		btnApplyAll->Enabled = false;
		/*this->picBoxCNRLeft->Image = nullptr;
		this->picBoxCNRRight->Image = nullptr;
		this->picBoxCTFLeft->Image = nullptr;
		this->picBoxCTFRight->Image = nullptr;*/
		m_calculateLUT = true;
		m_rotateUpdate = false;
	}

	//Draw Left and Right Area
	if (metric.widthLeft > 0 && metric.widthRight > 0)
	{
		m_rectLeft = System::Drawing::Rectangle((subROI.x + metric.xLeft) * m_scale, subROI.y * m_scale, metric.widthLeft * m_scale, subROI.height * m_scale);
		m_rectRight = System::Drawing::Rectangle((subROI.x + metric.xRight) * m_scale, subROI.y * m_scale, metric.widthRight * m_scale, subROI.height * m_scale);
	}

	System::Drawing::Rectangle rectBox = System::Drawing::Rectangle(0, 0, m_cum_width, m_cum_height);
	if (metric.CNRLeftList.size() > 0)
	{
		System::Drawing::Graphics^ grapCNRLeft = picBoxCNRLeft->CreateGraphics();
		Bitmap^ bitm = GraphDraw(grapCNRLeft, metric.xCoordinateLeftList, metric.CNRLeftList, "CNR Left Part", "CNR", typicalValue->CNR);
		grapCNRLeft->DrawImage(bitm, rectBox);
		bitm->~Bitmap();
	}
	if (metric.CTFLeftList.size() > 0)
	{
		System::Drawing::Graphics^ grapCTFLeft = picBoxCTFLeft->CreateGraphics();
		Bitmap^ bitm = GraphDraw(grapCTFLeft, metric.xCoordinateLeftList, metric.CTFLeftList, "CTF Left Part", "CTF(%)", typicalValue->CTF);
		grapCTFLeft->DrawImage(bitm, rectBox);
		bitm->~Bitmap();
	}
	if (metric.CNRRightList.size() > 0)
	{
		System::Drawing::Graphics^ grapCNRRight = picBoxCNRRight->CreateGraphics();
		Bitmap^ bitm = GraphDraw(grapCNRRight, metric.xCoordinateRightList, metric.CNRRightList, "CNR Right Part", "CNR", typicalValue->CNR);
		grapCNRRight->DrawImage(bitm, rectBox);
		bitm->~Bitmap();
	}
	if (metric.CTFRightList.size() > 0)
	{
		System::Drawing::Graphics^ grapCTFRight = picBoxCTFRight->CreateGraphics();
		Bitmap^ bitm = GraphDraw(grapCTFRight, metric.xCoordinateRightList, metric.CTFRightList, "CTF Right Part", "CTF(%)", typicalValue->CTF);
		grapCTFRight->DrawImage(bitm, rectBox);
		bitm->~Bitmap();
	}
	if (metric.cumulativeFunc.size() > 0)
	{
	    System::Drawing::Graphics^ grapCumulative = picBoxCumulative->CreateGraphics();
		Bitmap^ bitm = GraphDraw(grapCumulative, metric.cumulativeFunc, "Cumulative", "Intensity");
		System::Drawing::Rectangle rectBox1 = System::Drawing::Rectangle(0, 0, 830, 100);
		grapCumulative->DrawImage(bitm, rectBox1);
		bitm->~Bitmap();
	}

	CTFLeft = std::round(metric.CTFLeft * 100.0) / 100.0;
	CTFRight = std::round(metric.CTFRight * 100.0) / 100.0;
	CTFAvg = std::round((metric.CTFLeft + metric.CTFRight) / 2 * 100.0) / 100.0;
	SNRLeft = std::round(metric.SNRLeft * 100.0) / 100.0;
	SNRRight = std::round(metric.SNRRight * 100.0) / 100.0;
	SNRAvg = std::round((metric.SNRLeft + metric.SNRRight) / 2 * 100.0) / 100.0;
	CNRLeft = std::round(metric.CNRLeft * 100.0) / 100.0;
	CNRRight = std::round(metric.CNRRight * 100.0) / 100.0;
	CNRAvg = std::round((metric.CNRLeft + metric.CNRRight) / 2 * 100.0) / 100.0;

	
	m_isDraw = !m_isMove;
	System::Windows::Forms::DataGridViewRow^ rowSNR = this->dataGridView1->Rows[static_cast<int>(Items::SNR)];
	rowSNR->Cells[static_cast<int>(Area::LEFT)]->Value = SNRLeft;
	rowSNR->Cells[static_cast<int>(Area::RIGHT)]->Value = SNRRight;
	rowSNR->Cells[static_cast<int>(Area::AVERAGE)]->Value = SNRAvg;
	System::Windows::Forms::DataGridViewRow^ rowCTF = this->dataGridView1->Rows[static_cast<int>(Items::CTF)];
	rowCTF->Cells[static_cast<int>(Area::LEFT)]->Value = CTFLeft;
	rowCTF->Cells[static_cast<int>(Area::RIGHT)]->Value = CTFRight;
	rowCTF->Cells[static_cast<int>(Area::AVERAGE)]->Value = CTFAvg;
	System::Windows::Forms::DataGridViewRow^ rowCNR = this->dataGridView1->Rows[static_cast<int>(Items::CNR)];
	rowCNR->Cells[static_cast<int>(Area::LEFT)]->Value = CNRLeft;
	rowCNR->Cells[static_cast<int>(Area::RIGHT)]->Value = CNRRight;
	rowCNR->Cells[static_cast<int>(Area::AVERAGE)]->Value = CNRAvg;
}

void qualitymeasurement::MainForm::Reset()
{
	m_totalImage = 0;
	m_isDraw = false;
	m_isMove = false;
	m_isMouseDown = false;
	m_isMoving = false;
	m_imageIdx = 0;
	m_scale = 0;
	m_rect = System::Drawing::Rectangle(0, 0, 0, 0);
	m_rectLeft = System::Drawing::Rectangle(0, 0, 0, 0);
	m_rectRight = System::Drawing::Rectangle(0, 0, 0, 0);
	m_offsetPoint = System::Drawing::Point(0, 0);
	btnExportResult->Enabled = false;
	btnMove->Enabled = false;
	btnApplyAll->Enabled = false;
	btnNext->Enabled = false;
	btnPrevious->Enabled = false;
	strIndex->Text = L"000/000";
	strXY->Text = L"(0:0)";
	for (int i = 0; i < 3; i++)
	{
		System::Windows::Forms::DataGridViewRow^ row = this->dataGridView1->Rows[i];
		row->Cells[static_cast<int>(Area::LEFT)]->Value = 0;
		row->Cells[static_cast<int>(Area::RIGHT)]->Value = 0;
		row->Cells[static_cast<int>(Area::AVERAGE)]->Value = 0;
	}
	strWight->Text = L"0";
	strHeight->Text = L"0";
	this->pictureBox->Image = nullptr;
	this->picBoxCNRLeft->Image = nullptr;
	this->picBoxCNRRight->Image = nullptr;
	this->picBoxCTFLeft->Image = nullptr;
	this->picBoxCTFRight->Image = nullptr;
	subROI = cv::Rect(0,0,0,0);
	movePos = cv::Point(0,0);
	fnSystem = nullptr;
	if(m_image != nullptr)
		m_image->~Image();
	lstROI.clear();
}

void qualitymeasurement::MainForm::LoadTypicalValue()
{
	typicalValue = gcnew TypicalValue;
	typicalValue = (TypicalValue^)typicalValue->LoadFromXml(xmlFile);
	if (!typicalValue)
		typicalValue = gcnew TypicalValue;
}

System::Void qualitymeasurement::MainForm::pictureBox_MouseMove(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
{
	System::Drawing::Rectangle rect;
	
	if (m_isDraw)
	{
		pictureBox->Cursor = Cursors::Cross;
	}
	else
	{
		array<System::Drawing::Rectangle>^ rectList = gcnew array<System::Drawing::Rectangle>(8);
		//Top Left
		System::Drawing::Rectangle rect (m_rect.X - 4, m_rect.Y - 4, 9, 9);
		rectList[0] = rect;
		//Bottom Right
		rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
		rectList[1] = rect;
		//Bottom Left
		rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
		rectList[2] = rect;
		//Top Right
		rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y - 4, 9, 9);
		rectList[3] = rect;
		//Top
		rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y - 4, 9, 9);
		rectList[4] = rect;
		//Left
		rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
		rectList[5] = rect;
		//Bottom
		rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
		rectList[6] = rect;
		//Right
		rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
		rectList[7] = rect;

		for (int i = 0; i < 8; i++)
		{
			if (isMouseInsideRect(rectList[i], e))
			{
				if (!m_isMoving)
				{
					m_mouseInsideCorner = i;
					switch (m_mouseInsideCorner)
					{
					case 0:
						pictureBox->Cursor = Cursors::SizeNWSE;
						
						break;
					case 1:
						pictureBox->Cursor = Cursors::SizeNWSE;
						
						break;
					case 2:
						pictureBox->Cursor = Cursors::SizeNESW;
						
						break;
					case 3:
						pictureBox->Cursor = Cursors::SizeNESW;
						
						break;
					case 4:
						pictureBox->Cursor = Cursors::SizeNS;
						
						break;
					case 6:
						pictureBox->Cursor = Cursors::SizeNS;
						
						break;
					case 5:
						pictureBox->Cursor = Cursors::SizeWE;
						
						break;
					case 7:
						pictureBox->Cursor = Cursors::SizeWE;
						
						break;
					default:

						break;
					}
				break;
				}
			}
			else if (i == 7)
			{
				if (!m_isMoving)
				{
					m_mouseInsideCorner = -1;
					if (e->Location.X > m_rect.X && (e->Location.X < m_rect.X + m_rect.Width)
						&& e->Location.Y > m_rect.Y && (e->Location.Y < m_rect.Y + m_rect.Height))
					{
						pictureBox->Cursor = Cursors::SizeAll;
					}
					else
					{
						pictureBox->Cursor = Cursors::Default;
					}
				}
			}
		}		
	}

	if (m_isMouseDown && m_isDraw)
	{
		rect.Width = e->Location.X - drawPos.x;
		rect.Height = e->Location.Y - drawPos.y;
		rect.X = drawPos.x;
		rect.Y = drawPos.y;
		if (rect.Width < 0)
		{
			rect.X = e->Location.X;
			rect.Width = -rect.Width;
		}
		if (rect.Height < 0)
		{
			rect.Y = e->Location.Y;
			rect.Height = -rect.Height;
		}
	}
	else if (m_isMove && m_isMoving)
	{
		int diffX;
		int diffY;
		switch (m_mouseInsideCorner)
		{
		case 0: //Top Left
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X + diffX;
			rect.Y = m_rect.Y + diffY;
			rect.Width = m_rect.Width - diffX;
			rect.Height = m_rect.Height - diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 1: //Bottom Right
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X;
			rect.Y = m_rect.Y;
			rect.Width = m_rect.Width + diffX;
			rect.Height = m_rect.Height + diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 2: ////Bottom Left
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X + diffX;
			rect.Y = m_rect.Y;
			rect.Width = m_rect.Width - diffX;
			rect.Height = m_rect.Height + diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 3: //Top Right
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X;
			rect.Y = m_rect.Y + diffY;
			rect.Width = m_rect.Width + diffX;
			rect.Height = m_rect.Height - diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 4: //Top
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X;
			rect.Y = m_rect.Y + diffY;
			rect.Width = m_rect.Width;
			rect.Height = m_rect.Height - diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 5: //Left
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X + diffX;
			rect.Y = m_rect.Y;
			rect.Width = m_rect.Width - diffX;
			rect.Height = m_rect.Height;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 6: //Bottom
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X;
			rect.Y = m_rect.Y;
			rect.Width = m_rect.Width;
			rect.Height = m_rect.Height + diffY;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		case 7: //Right
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X;
			rect.Y = m_rect.Y;
			rect.Width = m_rect.Width + diffX;
			rect.Height = m_rect.Height;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
			
		case -1:
			diffX = e->Location.X - movePos.x;
			diffY = e->Location.Y - movePos.y;
			rect.X = m_rect.X + diffX;
			rect.Y = m_rect.Y + diffY;
			rect.Width = m_rect.Width;
			rect.Height = m_rect.Height;
			movePos = cv::Point(e->Location.X, e->Location.Y);
			break;
		}
		
	}

	if (rect.X > pictureBox->Width - rect.Width)
	{
		//rect.Width = pictureBox->Width - rect.X;
	}
	if (rect.Y > pictureBox->Height - rect.Height)
	{
		//rect.Height = pictureBox->Height - rect.Y;
	}
	if (rect.Width < 0)
	{
		rect.Width = 0;
	}
	if (rect.Height < 0)
	{
		rect.Height = 0;
	}

	if (rect.Width > 0 && rect.Height > 0)
	{
		if ((m_isMove && m_isMoving) || (m_isMouseDown && m_isDraw))
		{
			m_rect.X = rect.X;
			m_rect.Y = rect.Y;
			int imageXPosition = m_rect.X / m_scale;
			int imageYPosition = m_rect.Y / m_scale;
			strXY->Text = L"(" + imageXPosition + ":" + imageYPosition + ")";
		}
		
		m_rect.Width = rect.Width;
		m_rect.Height = rect.Height;
		int rectWight = m_rect.Width / m_scale;
		int rectHeight = m_rect.Height / m_scale;
		strWight->Text = L"" + rectWight;
		strHeight->Text = L"" + rectHeight;
		Refresh();
	}
}

System::Void qualitymeasurement::MainForm::pictureBox_MouseUp(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
{
	m_isMouseDown = false;
	if (m_totalImage <= 0) return;

	if (m_rect.Width > 0 && m_rect.Height > 0)
	{
		cv::Rect rect = cv::Rect(m_rect.X / m_scale, m_rect.Y / m_scale, m_rect.Width / m_scale, m_rect.Height / m_scale);
		cv::Size img_size = cv::Size(pictureBox->Image->Width, pictureBox->Image->Height);
		subROI = RoiRefinement(rect, img_size);
		Update();
	}

	m_isMoving = false;
	movePos = cv::Point(0, 0);
}

System::Void qualitymeasurement::MainForm::pictureBox_Paint(System::Object^ sender, System::Windows::Forms::PaintEventArgs^ e)
{
	System::Drawing::Pen^ redPen = gcnew System::Drawing::Pen(Color::Red, 2);
	if (m_isDraw)
	{
		redPen->DashPattern = gcnew array<float>{2.0F, 2.0F};	
	}
	else
	{
		if (m_rect.Height > 0 && m_rect.Height > 0)
		{
			System::Drawing::SolidBrush^ solidRedBrush = gcnew System::Drawing::SolidBrush(Color::Red);
			System::Drawing::Rectangle rect (m_rect.X - 4, m_rect.Y - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width/2 - 4, m_rect.Y + m_rect.Height - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
			rect =  System::Drawing::Rectangle(m_rect.X + m_rect.Width - 4, m_rect.Y + m_rect.Height/2 - 4, 9, 9);
			e->Graphics->FillRectangle(solidRedBrush, rect);
		}
	}
	e->Graphics->DrawRectangle(redPen, m_rect);
	
	if (radioButtonROI2Middle->Checked)
	{
	    System::Drawing::Pen^ cyanPen = gcnew System::Drawing::Pen(Color::Cyan, 2);
	    if (m_rectLeft.Width > 0)
	    {
		    e->Graphics->DrawRectangle(cyanPen, m_rectLeft);
	    }
	    if (m_rectRight.Width > 0)
	    {
		    e->Graphics->DrawRectangle(cyanPen, m_rectRight);
	    }
	}
}

System::Void qualitymeasurement::MainForm::btnMove_Click(System::Object^ sender, System::EventArgs^ e)
{
	if (m_totalImage <= 0) return;
	if (subROI.width <= 0 || subROI.height <= 0)
		return;

	m_isMove = !m_isMove;

	Update();
}


System::Void qualitymeasurement::MainForm::textBox1_TextChanged(System::Object^ sender, System::EventArgs^ e)
{
	Reset();
	if (System::IO::Directory::Exists(textBox1->Text))
	{
		array<System::String^>^ searchPattern = { "*.BMP","*.TIF"};
		int lenght = 0;
		for each (System::String ^ var in searchPattern)
		{
			array<System::String^>^ lstFile = System::IO::Directory::GetFiles(textBox1->Text, var, System::IO::SearchOption::TopDirectoryOnly);
			if (fnSystem != nullptr)
				lenght = fnSystem->Length;

			System::Array::Resize(fnSystem, lenght + lstFile->Length);
			lstFile->CopyTo(fnSystem, lenght);
		}
		if (lenght > 0)
			m_totalImage = fnSystem->Length;
	}
	if (m_totalImage > 0)
	{
		m_imageIdx = 0;
		lstROI.resize(m_totalImage, cv::Rect(0, 0, 0, 0));
		MarshalString(fnSystem[m_imageIdx], fileName);

		//cv::Mat image16; //= cv::imread(fileName, cv::IMREAD_ANYDEPTH);
		FILE* fp = _wfopen(fileName.c_str(), L"rb");
	    if (!fp)
	    {
	        
	    }
	    fseek(fp, 0, SEEK_END);
	    long sz = ftell(fp);
	    char* buf = new char[sz];
	    fseek(fp, 0, SEEK_SET);
	    long n = fread(buf, 1, sz, fp);
	    cv::_InputArray arr(buf, sz);
	    image16bit = cv::imdecode(arr, cv::IMREAD_ANYDEPTH);
	    delete[] buf;
	    fclose(fp);

		if (rotateFlag < 3)
		{
			cv::rotate(image16bit, image16bit, rotateFlag);
		}
		
		if (image16bit.type() == CV_16UC1)
		{	
			cv::Mat image8;// = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));
			xvt::Convert8Bits(image16bit, image8);
			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			m_image = bitmap;
		}
		else
		{
			m_image = BitmapConverter::ToBitmap(image16bit);
		}
		caculateScaleOffset();
	}

	Update();
}

System::Void qualitymeasurement::MainForm::btnNext_Click(System::Object^ sender, System::EventArgs^ e)
{
	if (m_imageIdx < m_totalImage - 1)
	{
		m_image->~Image();
		lstROI[m_imageIdx] = subROI;
		m_imageIdx++;
		subROI = lstROI[m_imageIdx];
		strXY->Text = L"(" + subROI.x + ":" + subROI.y + ")";
		strWight->Text = L"" + subROI.width;
		strHeight->Text = L"" + subROI.height;

		MarshalString(fnSystem[m_imageIdx], fileName);

		//cv::Mat image16; //= cv::imread(fileName, cv::IMREAD_ANYDEPTH);
		FILE* fp = _wfopen(fileName.c_str(), L"rb");
	    if (!fp)
	    {
	        
	    }
	    fseek(fp, 0, SEEK_END);
	    long sz = ftell(fp);
	    char* buf = new char[sz];
	    fseek(fp, 0, SEEK_SET);
	    long n = fread(buf, 1, sz, fp);
	    cv::_InputArray arr(buf, sz);
	    image16bit = cv::imdecode(arr, cv::IMREAD_ANYDEPTH);
	    delete[] buf;
	    fclose(fp);
		
		if (rotateFlag < 3)
		{
			cv::rotate(image16bit, image16bit, rotateFlag);
		}
		
		if (image16bit.type() == CV_16UC1)
		{
			cv::Mat image8;// = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));
			xvt::Convert8Bits(image16bit, image8);

			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			m_image = bitmap;
		}
		else
		{
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image16bit);
			m_image = bitmap;
		}
			
		//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
		caculateScaleOffset();
		m_rect = System::Drawing::Rectangle(subROI.x * m_scale, subROI.y * m_scale, subROI.width * m_scale, subROI.height * m_scale);
		Update();
	}
}

System::Void qualitymeasurement::MainForm::btnPrevious_Click(System::Object^ sender, System::EventArgs^ e)
{
	if (m_imageIdx > 0)
	{
		m_image->~Image();
		lstROI[m_imageIdx] = subROI;
		m_imageIdx--;
		subROI = lstROI[m_imageIdx];
		strXY->Text = L"(" + subROI.x + ":" + subROI.y + ")";
		strWight->Text = L"" + subROI.width;
		strHeight->Text = L"" + subROI.height;

		MarshalString(fnSystem[m_imageIdx], fileName);

		//cv::Mat image16; //= cv::imread(fileName, cv::IMREAD_ANYDEPTH);
		FILE* fp = _wfopen(fileName.c_str(), L"rb");
	    if (!fp)
	    {
	        
	    }
	    fseek(fp, 0, SEEK_END);
	    long sz = ftell(fp);
	    char* buf = new char[sz];
	    fseek(fp, 0, SEEK_SET);
	    long n = fread(buf, 1, sz, fp);
	    cv::_InputArray arr(buf, sz);
	    image16bit = cv::imdecode(arr, cv::IMREAD_ANYDEPTH);
	    delete[] buf;
	    fclose(fp);

		if (rotateFlag < 3)
		{
			cv::rotate(image16bit, image16bit, rotateFlag);
		}
		
		if (image16bit.type() == CV_16UC1)
		{
			cv::Mat image8;// = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));
			xvt::Convert8Bits(image16bit, image8);

			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			m_image = bitmap;
		}
		else
		{
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image16bit);
			m_image = bitmap;
		}
		//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
		caculateScaleOffset();
		m_rect = System::Drawing::Rectangle(subROI.x * m_scale, subROI.y * m_scale, subROI.width * m_scale, subROI.height * m_scale);
		Update();
	}
}

System::Void qualitymeasurement::MainForm::btnBrown1_Click(System::Object^ sender, System::EventArgs^ e)
{
	System::Windows::Forms::FolderBrowserDialog^ folderBrowserDialog = gcnew System::Windows::Forms::FolderBrowserDialog();

	if (folderBrowserDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		this->textBox1->Text = folderBrowserDialog->SelectedPath;
	}
}

System::Void qualitymeasurement::MainForm::btnExportResult_Click(System::Object^ sender, System::EventArgs^ e)
{
	if (m_totalImage <= 0) return;

	System::Windows::Forms::SaveFileDialog^ saveFileDialog = gcnew System::Windows::Forms::SaveFileDialog();
	saveFileDialog->Filter = "CSV file (*.csv)|*.csv";
	saveFileDialog->Title = "Save a csv file";
	if (saveFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		std::wstring filename = (const wchar_t*)(System::Runtime::InteropServices::Marshal::StringToHGlobalUni(saveFileDialog->FileName)).ToPointer();
		//open file for writing
		std::wofstream fw(filename, std::ofstream::out);

		//check if file was successfully opened for writing
		if (fw.is_open())
		{
			fw << "Index," << "Name," << "SNR left," << "SNR right," << "SNR avg," << "CTF left," << "CTF right," << "CTF avg," << "CNR left," << "CNR right," << "CNR avg," << "\n";
			//store array contents to text file
			for (int i = 0; i < m_totalImage; i++) {
				Metric metric;
				cv::Rect roi = lstROI[i];
				if (roi.width > 0 && roi.height > 0)
				{
					Image^ img = System::Drawing::Bitmap::FromFile(fnSystem[i]);
					Bitmap^ bmp1 = gcnew Bitmap(img);
					int h = bmp1->Height;
					int w = bmp1->Width;
					cv::Mat src = cv::Mat(h, w, CV_8UC3);
					BitmapConverter::ToMat(bmp1, src);
					cv::Mat dst;
					cvtColor(src, dst, cv::COLOR_RGB2GRAY);
					if(radioButtonROICertainArea->Checked)
					{
					    MetricCalculation(dst(roi), metric);
					}
					else
					{
					    MetricCalculationWholeRegion(dst(roi), metric);
					}
					img->~Image();
					bmp1->~Bitmap();
					src.release();
					dst.release();
				}
				std::wstring imgfile = (const wchar_t*)(System::Runtime::InteropServices::Marshal::StringToHGlobalUni(fnSystem[i])).ToPointer();
				std::size_t found = imgfile.find_last_of(L"/\\");
				std::wstring currentImage = imgfile.substr(found+1);
				fw << (i + 1) << "," << currentImage << "," << (metric.SNRLeft) << "," << (metric.SNRRight) << "," << (metric.SNRLeft + metric.SNRRight) / 2 << ","  << metric.CTFLeft << "," << metric.CTFRight << "," << (metric.CTFLeft + metric.CTFRight) / 2 << ","  << metric.CNRLeft << "," << metric.CNRRight << "," << (metric.CNRLeft + metric.CNRRight) / 2 << "\n";
			}
			//close the file after the writing operation is completed
			fw.close();
		}
	}
}

float zoomScale = 1;
float zoomStep = 0.1;
System::Void qualitymeasurement::MainForm::pictureBox_MouseWheel(System::Object^ sender, System::Windows::Forms::MouseEventArgs^ e)
{
	if (m_totalImage <= 0) return;

	if (e->X >= pictureBox->Location.X && e->X <= pictureBox->Location.X + pictureBox->Width
		&& e->Y >= pictureBox->Location.Y && e->Y <= pictureBox->Location.Y + pictureBox->Height)
	{
		//  flag = 1;
		// Override OnMouseWheel event, for zooming in/out with the scroll wheel
		if (pictureBox->Image != nullptr)
		{
			// If the mouse wheel is moved backward (Zoom in)
			if (e->Delta > 0)
			{
				zoomScale += zoomStep;
				if (zoomScale > 10) zoomScale = 10;
			}
			else //Zoom out
			{
				zoomScale -= zoomStep;
				if (zoomScale < 0.1) zoomScale = 0.1;
			}

			int old_w = pictureBox->Width;
			int old_h = pictureBox->Height;
			pictureBox->Width = m_picBoxOrgSize.Width * zoomScale;
			pictureBox->Height = m_picBoxOrgSize.Height * zoomScale;

			// Change the Scroll
			if (zoomScale > 1) {
				vScrollBar1->Enabled = true;
				hScrollBar1->Enabled = true;
				/*vScrollBar1->Maximum = ceil(zoomScale) * 19;
				vScrollBar1->LargeChange = MAX(floor(vScrollBar1->Maximum / 10), 1);
				hScrollBar1->Maximum = ceil(zoomScale) * 10;
				hScrollBar1->LargeChange = MAX(floor(hScrollBar1->Maximum / 10), 1);*/

				vScrollBar1->Maximum = pictureBox->Height - panel3->Height + m_offsetPoint.Y * 2;
				vScrollBar1->LargeChange = MAX(floor(vScrollBar1->Maximum / 20), 1);
				hScrollBar1->Maximum = pictureBox->Width - panel3->Width + m_offsetPoint.X * 2;
				hScrollBar1->LargeChange = MAX(floor(hScrollBar1->Maximum / 20), 1);

				//int x = e->X - panel3->Left;
				//int y = e->Y - panel3->Top;
				int x = e->X;
				int y = e->Y;
				int x_new = x + (pictureBox->Left - x) * ((float)pictureBox->Width / old_w);
				int y_new = y + (pictureBox->Top - y) * ((float)pictureBox->Height / old_h);

				/*vScrollBar1->Value = MIN(MAX((float)abs(panel3->Top - y_new) / (pictureBox->Height - panel3->Height) * vScrollBar1->Maximum, vScrollBar1->Minimum), vScrollBar1->Maximum);
				hScrollBar1->Value = MIN(MAX((float)abs(panel3->Left - x_new) / (pictureBox->Width - panel3->Width) * hScrollBar1->Maximum, hScrollBar1->Minimum), hScrollBar1->Maximum);*/

				vScrollBar1->Value = MIN(MAX(panel3->Top + m_offsetPoint.Y - y_new, vScrollBar1->Minimum), vScrollBar1->Maximum);
				hScrollBar1->Value = MIN(MAX(panel3->Left + m_offsetPoint.X - x_new, hScrollBar1->Minimum), hScrollBar1->Maximum);
			}
			else {
				vScrollBar1->Enabled = false;
				hScrollBar1->Enabled = false;
				/*vScrollBar1->Maximum = 0;
				vScrollBar1->LargeChange = 1;
				hScrollBar1->Maximum = 0;
				hScrollBar1->LargeChange = 1;*/

				pictureBox->Top = m_centerPanel.Y - pictureBox->Height / 2;
				pictureBox->Left = m_centerPanel.X - pictureBox->Width / 2;
			}
		}

		m_scale = (float)pictureBox->Width / m_image->Width;
		m_rect = System::Drawing::Rectangle(subROI.x * m_scale, subROI.y * m_scale, subROI.width * m_scale, subROI.height * m_scale);
		m_calculateLUT = false;
		Update();
	}

	this->OnMouseWheel(e);
}

System::Void qualitymeasurement::MainForm::btnApplyAll_Click(System::Object^ sender, System::EventArgs^ e)
{
	if (m_totalImage <= 0) return;
	
	for (int i = 0; i < m_totalImage; i++)
	{
		System::Drawing::Image^ img = System::Drawing::Bitmap::FromFile(fnSystem[i]);
		int w = img->Width;
		int h = img->Height;
		img->~Image();
		cv::Rect roi = subROI;
		roi = RoiRefinement(roi, cv::Size(w, h));
		lstROI[i] = roi;
	}
}

System::Void qualitymeasurement::MainForm::dataGridView1_CellEndEdit(System::Object^ sender, System::Windows::Forms::DataGridViewCellEventArgs^ e)
{
	if (e->ColumnIndex == 1)
	{
		switch (e->RowIndex)
		{
		case static_cast<int>(Items::SNR) :
			typicalValue->SNR = System::Convert::ToDouble(this->dataGridView1->Rows[static_cast<int>(Items::SNR)]->Cells[e->ColumnIndex]->Value->ToString());
			break;
		case static_cast<int>(Items::CTF) :
			typicalValue->CTF = System::Convert::ToDouble(this->dataGridView1->Rows[static_cast<int>(Items::CTF)]->Cells[e->ColumnIndex]->Value->ToString());
			break;
		case static_cast<int>(Items::CNR) :
			typicalValue->CNR = System::Convert::ToDouble(this->dataGridView1->Rows[static_cast<int>(Items::CNR)]->Cells[e->ColumnIndex]->Value->ToString());
			break;
		default:
			break;
		}
		Update();
	}
}

System::Void qualitymeasurement::MainForm::btnOpenFile_Click(System::Object^ sender, System::EventArgs^ e)
{
	System::Windows::Forms::OpenFileDialog^ openFile = gcnew System::Windows::Forms::OpenFileDialog();
	openFile->Filter = "(*.BMP,*.TIF)|*.BMP;*.TIF|(*.TIF)|*.TIF|(*.BMP)|*.BMP";

	// Allow the user to select multiple images.
	openFile->Multiselect = true;
	openFile->Title = "My Image Browser";
	System::Windows::Forms::DialogResult dialogResult =  openFile->ShowDialog();

	if (dialogResult == System::Windows::Forms::DialogResult::OK)
	{
		Reset();
		m_totalImage = openFile->FileNames->Length;
		fnSystem = openFile->FileNames;
	}

	if (m_totalImage > 0)
	{
		m_imageIdx = 0;
		lstROI.resize(m_totalImage, cv::Rect(0, 0, 0, 0));

		MarshalString(fnSystem[m_imageIdx], fileName);

		//cv::Mat image16; //= cv::imread(fileName, cv::IMREAD_ANYDEPTH);
		FILE* fp = _wfopen(fileName.c_str(), L"rb");
	    if (!fp)
	    {
	        
	    }
	    fseek(fp, 0, SEEK_END);
	    long sz = ftell(fp);
	    char* buf = new char[sz];
	    fseek(fp, 0, SEEK_SET);
	    long n = fread(buf, 1, sz, fp);
	    cv::_InputArray arr(buf, sz);
	    image16bit = cv::imdecode(arr, cv::IMREAD_ANYDEPTH);
	    delete[] buf;
	    fclose(fp);

		if (rotateFlag < 3)
		{
			cv::rotate(image16bit, image16bit, rotateFlag);
		}
		
		if (image16bit.type() == CV_16UC1)
		{
			cv::Mat image8;// = toneMapping(image16bit, cv::Rect(0,0, image16bit.cols -1, image16bit.rows - 1));
			xvt::Convert8Bits(image16bit, image8);

			//m_image = System::Drawing::Bitmap::FromFile(fnSystem[m_imageIdx]);
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image8);
			m_image = bitmap;
		}
		else
		{
			Bitmap^ bitmap = BitmapConverter::ToBitmap(image16bit);
			m_image = bitmap;
		}
		caculateScaleOffset();
	}

	Update();
}

System::Void qualitymeasurement::MainForm::vScrollBar1_ValueChanged(System::Object^ sender, System::EventArgs^ e)
{
	if (m_totalImage <= 0) return;

	int minPos = panel3->Top + panel3->Height - pictureBox->Height - m_offsetPoint.Y * 2;

	int diffH = panel3->Top - (float)vScrollBar1->Value;
	
	pictureBox->Top = MAX(MIN(diffH, panel3->Top) + m_offsetPoint.Y, minPos);
}

System::Void qualitymeasurement::MainForm::hScrollBar1_ValueChanged(System::Object^ sender, System::EventArgs^ e)
{
	if (m_totalImage <= 0) return;

	int minPos = panel3->Left + panel3->Width - pictureBox->Width - m_offsetPoint.X * 2;

	int diffH = panel3->Left - (float)hScrollBar1->Value;

	pictureBox->Left = MAX(MIN(diffH, panel3->Left) + m_offsetPoint.X, minPos);
}

System::Void qualitymeasurement::MainForm::bt_ExportTMO_Click(System::Object^ sender, System::EventArgs^ e)
{

	cv::Mat image16;
	std::wstring imageFileName;
	for (int i = 0; i < m_totalImage; i++)
	{
		MarshalString(fnSystem[i], imageFileName);
		FILE* fp = _wfopen(imageFileName.c_str(), L"rb");
		fseek(fp, 0, SEEK_END);
		long sz = ftell(fp);
		char* buf = new char[sz];
		fseek(fp, 0, SEEK_SET);
		long n = fread(buf, 1, sz, fp);
		cv::_InputArray arr(buf, sz);
		image16 = cv::imdecode(arr, cv::IMREAD_ANYDEPTH);
		delete[] buf;
		fclose(fp);
		if (rotateFlag < 3)
		{
			cv::rotate(image16, image16, rotateFlag);
		}
		
		if (image16.type() == CV_16UC1)
		{
			/*cv::Mat image8 = cv::Mat(image16.rows, image16.cols, CV_8UC1);
			if (LUT.size() == USHRT_MAX + 1)
			{
				uint8_t* ptr8 = image8.ptr<uint8_t>(0);
				const uint16_t* ptr16 = image16.ptr<uint16_t>(0);
				int size = image16.total();
				for (int i = 0; i < size; i++)
				{
					ptr8[i] = LUT[ptr16[i]];
				}
			}

			size_t lastindex = imageFileName.find_last_of(L".");
			imageFileName = imageFileName.substr(0, lastindex);
			std::wstring fileNameTemp = imageFileName + L".bmp";
			FILE* fp = _wfopen(fileNameTemp.c_str(), L"wb");
			std::vector<uchar> buf_out;
			cv::imencode(".bmp", image8, buf_out);
			uchar* buff = &buf_out[0];
			fwrite(buff, sizeof(uchar), buf_out.size(), fp);
			fclose(fp);
			image8.release();*/
		}
	}
	image16.release();
}

System::Void qualitymeasurement::MainForm::arrow_KeyDown(System::Object^ sender, System::Windows::Forms::KeyEventArgs^ e)
{
	switch (e->KeyCode)
	{
	case Keys::Up:
	case Keys::W:
	case Keys::NumPad8:
		if (m_rect.Y >= 1)
		{
			m_rect.Y --;
			Refresh();
			if (m_rect.Width > 0 && m_rect.Height > 0)
			{
				cv::Rect rect = cv::Rect(m_rect.X / m_scale, m_rect.Y / m_scale, m_rect.Width / m_scale, m_rect.Height / m_scale);
				cv::Size img_size = cv::Size(pictureBox->Image->Width, pictureBox->Image->Height);
				subROI = RoiRefinement(rect, img_size);
				Update();
			}
		}
		break;
	case Keys::Down:
	case Keys::S:
	case Keys::NumPad2:
		if (m_rect.Y >= 0) /*Todo: Check this condition*/
		{
			m_rect.Y ++;
			Refresh();
			if (m_rect.Width > 0 && m_rect.Height > 0)
			{
				cv::Rect rect = cv::Rect(m_rect.X / m_scale, m_rect.Y / m_scale, m_rect.Width / m_scale, m_rect.Height / m_scale);
				cv::Size img_size = cv::Size(pictureBox->Image->Width, pictureBox->Image->Height);
				subROI = RoiRefinement(rect, img_size);
				Update();
			}
		}
		break;
	case Keys::Left:
	case Keys::A:
	case Keys::NumPad4:
		if (m_rect.X >= 1)
		{
			m_rect.X --;
			Refresh();
			if (m_rect.Width > 0 && m_rect.Height > 0)
			{
				cv::Rect rect = cv::Rect(m_rect.X / m_scale, m_rect.Y / m_scale, m_rect.Width / m_scale, m_rect.Height / m_scale);
				cv::Size img_size = cv::Size(pictureBox->Image->Width, pictureBox->Image->Height);
				subROI = RoiRefinement(rect, img_size);
				Update();
			}
		}
		break;
	case Keys::Right:
	case Keys::D:
	case Keys::NumPad6:
		if (m_rect.X >= 0) /*Todo: Check this condition*/
		{
			m_rect.X ++;
			Refresh();
			if (m_rect.Width > 0 && m_rect.Height > 0)
			{
				cv::Rect rect = cv::Rect(m_rect.X / m_scale, m_rect.Y / m_scale, m_rect.Width / m_scale, m_rect.Height / m_scale);
				cv::Size img_size = cv::Size(pictureBox->Image->Width, pictureBox->Image->Height);
				subROI = RoiRefinement(rect, img_size);
				Update();
			}
		}
		break;
	default:
		break;
	}
}

bool qualitymeasurement::MainForm::isMouseInsideRect(System::Drawing::Rectangle rect, System::Windows::Forms::MouseEventArgs^ e)
{
	if (e->Location.X >= rect.X && (e->Location.X <= rect.X + rect.Width)
				&& e->Location.Y >= rect.Y && (e->Location.Y <= rect.Y + rect.Height))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void qualitymeasurement::MainForm::InitializeComponent(void)
{
	System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle1 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
	System::Windows::Forms::DataGridViewCellStyle^ dataGridViewCellStyle2 = (gcnew System::Windows::Forms::DataGridViewCellStyle());
	this->pictureBox = (gcnew System::Windows::Forms::PictureBox());
	this->panel1 = (gcnew System::Windows::Forms::Panel());
	this->strIndex = (gcnew System::Windows::Forms::Label());
	this->label7 = (gcnew System::Windows::Forms::Label());
	this->textBox1 = (gcnew System::Windows::Forms::TextBox());
	this->label5 = (gcnew System::Windows::Forms::Label());
	this->btnBrown1 = (gcnew System::Windows::Forms::Button());
	this->btnPrevious = (gcnew System::Windows::Forms::Button());
	this->btnNext = (gcnew System::Windows::Forms::Button());
	this->btnOpenFile = (gcnew System::Windows::Forms::Button());
	this->bt_ExportTMO = (gcnew System::Windows::Forms::Button());
	this->btnExportResult = (gcnew System::Windows::Forms::Button());
	this->btnMove = (gcnew System::Windows::Forms::Button());
	this->btnApplyAll = (gcnew System::Windows::Forms::Button());
	this->strWight = (gcnew System::Windows::Forms::Label());
	this->strXY = (gcnew System::Windows::Forms::Label());
	this->strHeight = (gcnew System::Windows::Forms::Label());
	this->label9 = (gcnew System::Windows::Forms::Label());
	this->label3 = (gcnew System::Windows::Forms::Label());
	this->label2 = (gcnew System::Windows::Forms::Label());
	this->panel2 = (gcnew System::Windows::Forms::Panel());
	this->label6 = (gcnew System::Windows::Forms::Label());
	this->radioButtonROICertainArea = (gcnew System::Windows::Forms::RadioButton());
	this->radioButtonROI2Middle = (gcnew System::Windows::Forms::RadioButton());
	this->label4 = (gcnew System::Windows::Forms::Label());
	this->label1 = (gcnew System::Windows::Forms::Label());
	this->textBox3 = (gcnew System::Windows::Forms::TextBox());
	this->textBox2 = (gcnew System::Windows::Forms::TextBox());
	this->dataGridView1 = (gcnew System::Windows::Forms::DataGridView());
	this->Column1 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
	this->Column2 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
	this->Column3 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
	this->Column4 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
	this->Column5 = (gcnew System::Windows::Forms::DataGridViewTextBoxColumn());
	this->label14 = (gcnew System::Windows::Forms::Label());
	this->label13 = (gcnew System::Windows::Forms::Label());
	this->label12 = (gcnew System::Windows::Forms::Label());
	this->label11 = (gcnew System::Windows::Forms::Label());
	this->picBoxCNRRight = (gcnew System::Windows::Forms::PictureBox());
	this->picBoxCNRLeft = (gcnew System::Windows::Forms::PictureBox());
	this->picBoxCTFRight = (gcnew System::Windows::Forms::PictureBox());
	this->label10 = (gcnew System::Windows::Forms::Label());
	this->label8 = (gcnew System::Windows::Forms::Label());
	this->picBoxCTFLeft = (gcnew System::Windows::Forms::PictureBox());
	this->panel3 = (gcnew System::Windows::Forms::Panel());
	this->vScrollBar1 = (gcnew System::Windows::Forms::VScrollBar());
	this->hScrollBar1 = (gcnew System::Windows::Forms::HScrollBar());
	this->picBoxCumulative = (gcnew System::Windows::Forms::PictureBox());
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox))->BeginInit();
	this->panel1->SuspendLayout();
	this->panel2->SuspendLayout();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dataGridView1))->BeginInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCNRRight))->BeginInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCNRLeft))->BeginInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCTFRight))->BeginInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCTFLeft))->BeginInit();
	this->panel3->SuspendLayout();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCumulative))->BeginInit();
	this->SuspendLayout();
	// 
	// pictureBox
	// 
	this->pictureBox->BackColor = System::Drawing::Color::White;
	this->pictureBox->Location = System::Drawing::Point(0, 0);
	this->pictureBox->Name = L"pictureBox";
	this->pictureBox->Size = System::Drawing::Size(828, 740);
	this->pictureBox->SizeMode = System::Windows::Forms::PictureBoxSizeMode::Zoom;
	this->pictureBox->TabIndex = 1;
	this->pictureBox->TabStop = false;
	this->pictureBox->Paint += gcnew System::Windows::Forms::PaintEventHandler(this, &MainForm::pictureBox_Paint);
	this->pictureBox->MouseDown += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::pictureBox_MouseDown);
	this->pictureBox->MouseMove += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::pictureBox_MouseMove);
	this->pictureBox->MouseUp += gcnew System::Windows::Forms::MouseEventHandler(this, &MainForm::pictureBox_MouseUp);
	// 
	// panel1
	// 
	this->panel1->Controls->Add(this->strIndex);
	this->panel1->Controls->Add(this->label7);
	this->panel1->Controls->Add(this->textBox1);
	this->panel1->Controls->Add(this->label5);
	this->panel1->Controls->Add(this->btnBrown1);
	this->panel1->Controls->Add(this->btnPrevious);
	this->panel1->Controls->Add(this->btnNext);
	this->panel1->Controls->Add(this->btnOpenFile);
	this->panel1->Controls->Add(this->bt_ExportTMO);
	this->panel1->Dock = System::Windows::Forms::DockStyle::Bottom;
	this->panel1->Location = System::Drawing::Point(0, 872);
	this->panel1->Name = L"panel1";
	this->panel1->Size = System::Drawing::Size(1185, 59);
	this->panel1->TabIndex = 2;
	// 
	// strIndex
	// 
	this->strIndex->AutoSize = true;
	this->strIndex->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->strIndex->Location = System::Drawing::Point(77, 35);
	this->strIndex->Name = L"strIndex";
	this->strIndex->Size = System::Drawing::Size(67, 17);
	this->strIndex->TabIndex = 19;
	this->strIndex->Text = L"000/000";
	// 
	// label7
	// 
	this->label7->AutoSize = true;
	this->label7->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 11.25F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label7->Location = System::Drawing::Point(25, 34);
	this->label7->Name = L"label7";
	this->label7->Size = System::Drawing::Size(46, 18);
	this->label7->TabIndex = 19;
	this->label7->Text = L"Index:";
	// 
	// textBox1
	// 
	this->textBox1->Location = System::Drawing::Point(169, 3);
	this->textBox1->Name = L"textBox1";
	this->textBox1->Size = System::Drawing::Size(416, 20);
	this->textBox1->TabIndex = 5;
	this->textBox1->TextChanged += gcnew System::EventHandler(this, &MainForm::textBox1_TextChanged);
	// 
	// label5
	// 
	this->label5->AutoSize = true;
	this->label5->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label5->Location = System::Drawing::Point(19, 5);
	this->label5->Name = L"label5";
	this->label5->Size = System::Drawing::Size(135, 17);
	this->label5->TabIndex = 4;
	this->label5->Text = L"Choose Source File:";
	// 
	// btnBrown1
	// 
	this->btnBrown1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnBrown1->Location = System::Drawing::Point(510, 29);
	this->btnBrown1->Name = L"btnBrown1";
	this->btnBrown1->Size = System::Drawing::Size(75, 23);
	this->btnBrown1->TabIndex = 6;
	this->btnBrown1->Text = L"Browse...";
	this->btnBrown1->UseVisualStyleBackColor = true;
	this->btnBrown1->Click += gcnew System::EventHandler(this, &MainForm::btnBrown1_Click);
	// 
	// btnPrevious
	// 
	this->btnPrevious->Enabled = false;
	this->btnPrevious->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnPrevious->Location = System::Drawing::Point(966, 12);
	this->btnPrevious->Name = L"btnPrevious";
	this->btnPrevious->Size = System::Drawing::Size(90, 36);
	this->btnPrevious->TabIndex = 0;
	this->btnPrevious->Text = L"Previous";
	this->btnPrevious->UseVisualStyleBackColor = true;
	this->btnPrevious->Click += gcnew System::EventHandler(this, &MainForm::btnPrevious_Click);
	// 
	// btnNext
	// 
	this->btnNext->Enabled = false;
	this->btnNext->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnNext->Location = System::Drawing::Point(1075, 12);
	this->btnNext->Name = L"btnNext";
	this->btnNext->Size = System::Drawing::Size(90, 36);
	this->btnNext->TabIndex = 0;
	this->btnNext->Text = L"Next";
	this->btnNext->UseVisualStyleBackColor = true;
	this->btnNext->Click += gcnew System::EventHandler(this, &MainForm::btnNext_Click);
	// 
	// btnOpenFile
	// 
	this->btnOpenFile->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnOpenFile->Location = System::Drawing::Point(857, 12);
	this->btnOpenFile->Name = L"btnOpenFile";
	this->btnOpenFile->Size = System::Drawing::Size(90, 36);
	this->btnOpenFile->TabIndex = 0;
	this->btnOpenFile->Text = L"Open File";
	this->btnOpenFile->UseVisualStyleBackColor = true;
	this->btnOpenFile->Click += gcnew System::EventHandler(this, &MainForm::btnOpenFile_Click);
	// 
	// bt_ExportTMO
	// 
	this->bt_ExportTMO->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->bt_ExportTMO->Location = System::Drawing::Point(619, 16);
	this->bt_ExportTMO->Name = L"bt_ExportTMO";
	this->bt_ExportTMO->Size = System::Drawing::Size(90, 36);
	this->bt_ExportTMO->TabIndex = 0;
	this->bt_ExportTMO->Text = L"Export TMO";
	this->bt_ExportTMO->UseVisualStyleBackColor = true;
	this->bt_ExportTMO->Click += gcnew System::EventHandler(this, &MainForm::bt_ExportTMO_Click);
	// 
	// btnExportResult
	// 
	this->btnExportResult->Enabled = false;
	this->btnExportResult->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnExportResult->Location = System::Drawing::Point(225, 836);
	this->btnExportResult->Name = L"btnExportResult";
	this->btnExportResult->Size = System::Drawing::Size(90, 36);
	this->btnExportResult->TabIndex = 0;
	this->btnExportResult->Text = L"Export Result";
	this->btnExportResult->UseVisualStyleBackColor = true;
	this->btnExportResult->Click += gcnew System::EventHandler(this, &MainForm::btnExportResult_Click);
	// 
	// btnMove
	// 
	this->btnMove->Enabled = false;
	this->btnMove->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnMove->Location = System::Drawing::Point(7, 836);
	this->btnMove->Name = L"btnMove";
	this->btnMove->Size = System::Drawing::Size(90, 36);
	this->btnMove->TabIndex = 0;
	this->btnMove->Text = L"Move";
	this->btnMove->UseVisualStyleBackColor = true;
	this->btnMove->Click += gcnew System::EventHandler(this, &MainForm::btnMove_Click);
	// 
	// btnApplyAll
	// 
	this->btnApplyAll->Enabled = false;
	this->btnApplyAll->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.5F, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->btnApplyAll->Location = System::Drawing::Point(116, 836);
	this->btnApplyAll->Name = L"btnApplyAll";
	this->btnApplyAll->Size = System::Drawing::Size(90, 36);
	this->btnApplyAll->TabIndex = 0;
	this->btnApplyAll->Text = L"Apply to All";
	this->btnApplyAll->UseVisualStyleBackColor = true;
	this->btnApplyAll->Click += gcnew System::EventHandler(this, &MainForm::btnApplyAll_Click);
	// 
	// strWight
	// 
	this->strWight->AutoSize = true;
	this->strWight->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->strWight->Location = System::Drawing::Point(328, 708);
	this->strWight->Name = L"strWight";
	this->strWight->Size = System::Drawing::Size(17, 17);
	this->strWight->TabIndex = 2;
	this->strWight->Text = L"0";
	// 
	// strXY
	// 
	this->strXY->AutoSize = true;
	this->strXY->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->strXY->Location = System::Drawing::Point(62, 708);
	this->strXY->Name = L"strXY";
	this->strXY->Size = System::Drawing::Size(43, 17);
	this->strXY->TabIndex = 2;
	this->strXY->Text = L"(0:0)";
	// 
	// strHeight
	// 
	this->strHeight->AutoSize = true;
	this->strHeight->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->strHeight->Location = System::Drawing::Point(231, 708);
	this->strHeight->Name = L"strHeight";
	this->strHeight->Size = System::Drawing::Size(17, 17);
	this->strHeight->TabIndex = 2;
	this->strHeight->Text = L"0";
	// 
	// label9
	// 
	this->label9->AutoSize = true;
	this->label9->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label9->Location = System::Drawing::Point(277, 708);
	this->label9->Name = L"label9";
	this->label9->Size = System::Drawing::Size(59, 17);
	this->label9->TabIndex = 2;
	this->label9->Text = L"Width: ";
	// 
	// label3
	// 
	this->label3->AutoSize = true;
	this->label3->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label3->Location = System::Drawing::Point(174, 708);
	this->label3->Name = L"label3";
	this->label3->Size = System::Drawing::Size(65, 17);
	this->label3->TabIndex = 2;
	this->label3->Text = L"Height: ";
	// 
	// label2
	// 
	this->label2->AutoSize = true;
	this->label2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label2->Location = System::Drawing::Point(18, 708);
	this->label2->Name = L"label2";
	this->label2->Size = System::Drawing::Size(45, 17);
	this->label2->TabIndex = 2;
	this->label2->Text = L"(x,y):";
	// 
	// panel2
	// 
	this->panel2->Controls->Add(this->label6);
	this->panel2->Controls->Add(this->radioButtonROICertainArea);
	this->panel2->Controls->Add(this->radioButtonROI2Middle);
	this->panel2->Controls->Add(this->label4);
	this->panel2->Controls->Add(this->label1);
	this->panel2->Controls->Add(this->textBox3);
	this->panel2->Controls->Add(this->textBox2);
	this->panel2->Controls->Add(this->dataGridView1);
	this->panel2->Controls->Add(this->label14);
	this->panel2->Controls->Add(this->label13);
	this->panel2->Controls->Add(this->label12);
	this->panel2->Controls->Add(this->btnExportResult);
	this->panel2->Controls->Add(this->label11);
	this->panel2->Controls->Add(this->picBoxCNRRight);
	this->panel2->Controls->Add(this->picBoxCNRLeft);
	this->panel2->Controls->Add(this->picBoxCTFRight);
	this->panel2->Controls->Add(this->btnMove);
	this->panel2->Controls->Add(this->btnApplyAll);
	this->panel2->Controls->Add(this->label10);
	this->panel2->Controls->Add(this->label8);
	this->panel2->Controls->Add(this->picBoxCTFLeft);
	this->panel2->Dock = System::Windows::Forms::DockStyle::Right;
	this->panel2->Location = System::Drawing::Point(850, 0);
	this->panel2->Name = L"panel2";
	this->panel2->Size = System::Drawing::Size(335, 872);
	this->panel2->TabIndex = 4;
	// 
	// label6
	// 
	this->label6->AutoSize = true;
	this->label6->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label6->Location = System::Drawing::Point(12, 774);
	this->label6->Name = L"label6";
	this->label6->Size = System::Drawing::Size(67, 17);
	this->label6->TabIndex = 25;
	this->label6->Text = L"ROI type:";
	// 
	// radioButtonROICertainArea
	// 
	this->radioButtonROICertainArea->AutoSize = true;
	this->radioButtonROICertainArea->Location = System::Drawing::Point(225, 774);
	this->radioButtonROICertainArea->Name = L"radioButtonROICertainArea";
	this->radioButtonROICertainArea->Size = System::Drawing::Size(80, 17);
	this->radioButtonROICertainArea->TabIndex = 24;
	this->radioButtonROICertainArea->Text = L"CertainArea";
	this->radioButtonROICertainArea->UseVisualStyleBackColor = true;
	this->radioButtonROICertainArea->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButton2_CheckedChanged);
	// 
	// radioButtonROI2Middle
	// 
	this->radioButtonROI2Middle->AutoSize = true;
	this->radioButtonROI2Middle->Checked = true;
	this->radioButtonROI2Middle->Location = System::Drawing::Point(99, 774);
	this->radioButtonROI2Middle->Name = L"radioButtonROI2Middle";
	this->radioButtonROI2Middle->Size = System::Drawing::Size(108, 17);
	this->radioButtonROI2Middle->TabIndex = 23;
	this->radioButtonROI2Middle->TabStop = true;
	this->radioButtonROI2Middle->Text = L"2 middle of region";
	this->radioButtonROI2Middle->UseVisualStyleBackColor = true;
	this->radioButtonROI2Middle->CheckedChanged += gcnew System::EventHandler(this, &MainForm::radioButton1_CheckedChanged);
	// 
	// label4
	// 
	this->label4->AutoSize = true;
	this->label4->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label4->Location = System::Drawing::Point(171, 799);
	this->label4->Name = L"label4";
	this->label4->Size = System::Drawing::Size(78, 17);
	this->label4->TabIndex = 22;
	this->label4->Text = L"Offset (px):";
	// 
	// label1
	// 
	this->label1->AutoSize = true;
	this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label1->Location = System::Drawing::Point(11, 798);
	this->label1->Name = L"label1";
	this->label1->Size = System::Drawing::Size(82, 17);
	this->label1->TabIndex = 20;
	this->label1->Text = L"Center (px):";
	// 
	// textBox3
	// 
	this->textBox3->Location = System::Drawing::Point(251, 797);
	this->textBox3->Name = L"textBox3";
	this->textBox3->Size = System::Drawing::Size(64, 20);
	this->textBox3->TabIndex = 21;
	// 
	// textBox2
	// 
	this->textBox2->Location = System::Drawing::Point(99, 797);
	this->textBox2->Name = L"textBox2";
	this->textBox2->Size = System::Drawing::Size(66, 20);
	this->textBox2->TabIndex = 20;
	// 
	// dataGridView1
	// 
	this->dataGridView1->AllowUserToAddRows = false;
	this->dataGridView1->AllowUserToDeleteRows = false;
	this->dataGridView1->AllowUserToResizeColumns = false;
	this->dataGridView1->AllowUserToResizeRows = false;
	dataGridViewCellStyle1->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
	dataGridViewCellStyle1->BackColor = System::Drawing::SystemColors::Control;
	dataGridViewCellStyle1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	dataGridViewCellStyle1->ForeColor = System::Drawing::SystemColors::WindowText;
	dataGridViewCellStyle1->SelectionBackColor = System::Drawing::SystemColors::Highlight;
	dataGridViewCellStyle1->SelectionForeColor = System::Drawing::SystemColors::HighlightText;
	dataGridViewCellStyle1->WrapMode = System::Windows::Forms::DataGridViewTriState::True;
	this->dataGridView1->ColumnHeadersDefaultCellStyle = dataGridViewCellStyle1;
	this->dataGridView1->ColumnHeadersHeightSizeMode = System::Windows::Forms::DataGridViewColumnHeadersHeightSizeMode::AutoSize;
	this->dataGridView1->Columns->AddRange(gcnew cli::array< System::Windows::Forms::DataGridViewColumn^  >(5)
	{
		this->Column1,
			this->Column2, this->Column3, this->Column4, this->Column5
	});
	this->dataGridView1->Location = System::Drawing::Point(15, 12);
	this->dataGridView1->MultiSelect = false;
	this->dataGridView1->Name = L"dataGridView1";
	this->dataGridView1->RowHeadersVisible = false;
	this->dataGridView1->RowHeadersWidth = 50;
	dataGridViewCellStyle2->Alignment = System::Windows::Forms::DataGridViewContentAlignment::MiddleCenter;
	dataGridViewCellStyle2->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->dataGridView1->RowsDefaultCellStyle = dataGridViewCellStyle2;
	this->dataGridView1->Size = System::Drawing::Size(300, 90);
	this->dataGridView1->TabIndex = 10;
	this->dataGridView1->CellEndEdit += gcnew System::Windows::Forms::DataGridViewCellEventHandler(this, &MainForm::dataGridView1_CellEndEdit);
	// 
	// Column1
	// 
	this->Column1->FillWeight = 97;
	this->Column1->HeaderText = L"Items";
	this->Column1->Name = L"Column1";
	this->Column1->ReadOnly = true;
	this->Column1->Resizable = System::Windows::Forms::DataGridViewTriState::False;
	this->Column1->Width = 97;
	// 
	// Column2
	// 
	this->Column2->FillWeight = 50;
	this->Column2->HeaderText = L"Typical";
	this->Column2->Name = L"Column2";
	this->Column2->Resizable = System::Windows::Forms::DataGridViewTriState::False;
	this->Column2->Width = 50;
	// 
	// Column3
	// 
	this->Column3->FillWeight = 50;
	this->Column3->HeaderText = L"Left";
	this->Column3->Name = L"Column3";
	this->Column3->ReadOnly = true;
	this->Column3->Resizable = System::Windows::Forms::DataGridViewTriState::False;
	this->Column3->Width = 50;
	// 
	// Column4
	// 
	this->Column4->FillWeight = 50;
	this->Column4->HeaderText = L"Right";
	this->Column4->Name = L"Column4";
	this->Column4->ReadOnly = true;
	this->Column4->Resizable = System::Windows::Forms::DataGridViewTriState::False;
	this->Column4->Width = 50;
	// 
	// Column5
	// 
	this->Column5->FillWeight = 50;
	this->Column5->HeaderText = L"Avg";
	this->Column5->Name = L"Column5";
	this->Column5->ReadOnly = true;
	this->Column5->Resizable = System::Windows::Forms::DataGridViewTriState::False;
	this->Column5->Width = 50;
	// 
	// label14
	// 
	this->label14->AutoSize = true;
	this->label14->BackColor = System::Drawing::Color::White;
	this->label14->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label14->Location = System::Drawing::Point(17, 575);
	this->label14->Name = L"label14";
	this->label14->Size = System::Drawing::Size(33, 13);
	this->label14->TabIndex = 3;
	this->label14->Text = L"CNR";
	// 
	// label13
	// 
	this->label13->AutoSize = true;
	this->label13->BackColor = System::Drawing::Color::White;
	this->label13->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label13->Location = System::Drawing::Point(17, 426);
	this->label13->Name = L"label13";
	this->label13->Size = System::Drawing::Size(30, 13);
	this->label13->TabIndex = 3;
	this->label13->Text = L"CTF";
	// 
	// label12
	// 
	this->label12->AutoSize = true;
	this->label12->BackColor = System::Drawing::Color::White;
	this->label12->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label12->Location = System::Drawing::Point(17, 276);
	this->label12->Name = L"label12";
	this->label12->Size = System::Drawing::Size(33, 13);
	this->label12->TabIndex = 3;
	this->label12->Text = L"CNR";
	// 
	// label11
	// 
	this->label11->AutoSize = true;
	this->label11->BackColor = System::Drawing::Color::White;
	this->label11->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label11->Location = System::Drawing::Point(17, 126);
	this->label11->Name = L"label11";
	this->label11->Size = System::Drawing::Size(30, 13);
	this->label11->TabIndex = 3;
	this->label11->Text = L"CTF";
	// 
	// picBoxCNRRight
	// 
	this->picBoxCNRRight->BackColor = System::Drawing::Color::White;
	this->picBoxCNRRight->Location = System::Drawing::Point(15, 572);
	this->picBoxCNRRight->Name = L"picBoxCNRRight";
	this->picBoxCNRRight->Size = System::Drawing::Size(300, 130);
	this->picBoxCNRRight->TabIndex = 9;
	this->picBoxCNRRight->TabStop = false;
	// 
	// picBoxCNRLeft
	// 
	this->picBoxCNRLeft->BackColor = System::Drawing::Color::White;
	this->picBoxCNRLeft->Location = System::Drawing::Point(15, 272);
	this->picBoxCNRLeft->Name = L"picBoxCNRLeft";
	this->picBoxCNRLeft->Size = System::Drawing::Size(300, 130);
	this->picBoxCNRLeft->TabIndex = 9;
	this->picBoxCNRLeft->TabStop = false;
	// 
	// picBoxCTFRight
	// 
	this->picBoxCTFRight->BackColor = System::Drawing::Color::White;
	this->picBoxCTFRight->Location = System::Drawing::Point(15, 422);
	this->picBoxCTFRight->Name = L"picBoxCTFRight";
	this->picBoxCTFRight->Size = System::Drawing::Size(300, 130);
	this->picBoxCTFRight->TabIndex = 9;
	this->picBoxCTFRight->TabStop = false;
	// 
	// label10
	// 
	this->label10->AutoSize = true;
	this->label10->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label10->Location = System::Drawing::Point(12, 404);
	this->label10->Name = L"label10";
	this->label10->Size = System::Drawing::Size(77, 17);
	this->label10->TabIndex = 4;
	this->label10->Text = L"Right Side:";
	// 
	// label8
	// 
	this->label8->AutoSize = true;
	this->label8->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 10, System::Drawing::FontStyle::Regular, System::Drawing::GraphicsUnit::Point,
		static_cast<System::Byte>(0)));
	this->label8->Location = System::Drawing::Point(12, 104);
	this->label8->Name = L"label8";
	this->label8->Size = System::Drawing::Size(68, 17);
	this->label8->TabIndex = 4;
	this->label8->Text = L"Left Side:";
	// 
	// picBoxCTFLeft
	// 
	this->picBoxCTFLeft->BackColor = System::Drawing::Color::White;
	this->picBoxCTFLeft->Location = System::Drawing::Point(15, 122);
	this->picBoxCTFLeft->Name = L"picBoxCTFLeft";
	this->picBoxCTFLeft->Size = System::Drawing::Size(300, 130);
	this->picBoxCTFLeft->TabIndex = 9;
	this->picBoxCTFLeft->TabStop = false;
	// 
	// panel3
	// 
	this->panel3->BackColor = System::Drawing::Color::FromArgb(static_cast<System::Int32>(static_cast<System::Byte>(64)), static_cast<System::Int32>(static_cast<System::Byte>(64)),
		static_cast<System::Int32>(static_cast<System::Byte>(64)));
	this->panel3->Controls->Add(this->pictureBox);
	this->panel3->Location = System::Drawing::Point(0, 0);
	this->panel3->Name = L"panel3";
	this->panel3->Size = System::Drawing::Size(828, 740);
	this->panel3->TabIndex = 5;
	// 
	// vScrollBar1
	// 
	this->vScrollBar1->Enabled = false;
	this->vScrollBar1->LargeChange = 20;
	this->vScrollBar1->Location = System::Drawing::Point(833, 2);
	this->vScrollBar1->Name = L"vScrollBar1";
	this->vScrollBar1->Size = System::Drawing::Size(17, 738);
	this->vScrollBar1->TabIndex = 2;
	this->vScrollBar1->Value = 20;
	this->vScrollBar1->ValueChanged += gcnew System::EventHandler(this, &MainForm::vScrollBar1_ValueChanged);
	// 
	// hScrollBar1
	// 
	this->hScrollBar1->Enabled = false;
	this->hScrollBar1->Location = System::Drawing::Point(0, 743);
	this->hScrollBar1->Name = L"hScrollBar1";
	this->hScrollBar1->Size = System::Drawing::Size(832, 17);
	this->hScrollBar1->TabIndex = 6;
	this->hScrollBar1->ValueChanged += gcnew System::EventHandler(this, &MainForm::hScrollBar1_ValueChanged);
	// 
	// picBoxCumulative
	// 
	this->picBoxCumulative->BackColor = System::Drawing::Color::White;
	this->picBoxCumulative->Location = System::Drawing::Point(0, 765);
	this->picBoxCumulative->Name = L"picBoxCumulative";
	this->picBoxCumulative->Size = System::Drawing::Size(832, 101);
	this->picBoxCumulative->TabIndex = 11;
	this->picBoxCumulative->TabStop = false;
	// 
	// MainForm
	// 
	this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
	this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
	this->ClientSize = System::Drawing::Size(1185, 931);
	this->Controls->Add(this->picBoxCumulative);
	this->Controls->Add(this->hScrollBar1);
	this->Controls->Add(this->vScrollBar1);
	this->Controls->Add(this->panel2);
	this->Controls->Add(this->strWight);
	this->Controls->Add(this->strXY);
	this->Controls->Add(this->strHeight);
	this->Controls->Add(this->label9);
	this->Controls->Add(this->label2);
	this->Controls->Add(this->label3);
	this->Controls->Add(this->panel3);
	this->Controls->Add(this->panel1);
	this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
	this->KeyPreview = true;
	this->MaximizeBox = false;
	this->MinimizeBox = false;
	this->Name = L"MainForm";
	this->StartPosition = System::Windows::Forms::FormStartPosition::CenterScreen;
	this->Text = L"Image Quality Measurement";
	this->KeyDown += gcnew System::Windows::Forms::KeyEventHandler(this, &MainForm::arrow_KeyDown);
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox))->EndInit();
	this->panel1->ResumeLayout(false);
	this->panel1->PerformLayout();
	this->panel2->ResumeLayout(false);
	this->panel2->PerformLayout();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->dataGridView1))->EndInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCNRRight))->EndInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCNRLeft))->EndInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCTFRight))->EndInit();
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCTFLeft))->EndInit();
	this->panel3->ResumeLayout(false);
	(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->picBoxCumulative))->EndInit();
	this->ResumeLayout(false);
	this->PerformLayout();

}

