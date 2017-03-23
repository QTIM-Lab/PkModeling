/*=auto=========================================================================

  Portions (c) Copyright 2009 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See Doc/copyright/copyright.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Program:   3D Slicer
  Module:    $RCSfile: pk_solver.h,v $
  Date:      $Date: 2006/03/19 17:12:29 $
  Version:   $Revision: 1.13 $

  =========================================================================auto=*/

#ifndef PkSolver_h_
#define PkSolver_h_

#include "itkLevenbergMarquardtOptimizer.h"
#include "itkAmoebaOptimizer.h"
#include <math.h>
#include <vnl/algo/vnl_convolve.h>
#include "itkArray.h"
#include <string>

// work around compile error on Win
#define M_PI 3.1415926535897932384626433832795

// codes defined in ITKv4 vnl_levenberg_marquardt.cxx:386
// These diagnoistic codes will likely not work for the Amoeba optimizer?
// Need to check - not sure how Diagnoistic codes work.
enum OptimizerDiagnosticCodes {
  ERROR_FAILURE = 0,
  ERROR_DODGY_INPUT = 1,
  CONVERGED_FTOL = 2,
  CONVERGED_XTOL = 3,
  CONVERGED_XFTOL = 4,
  CONVERGED_GTOL = 5,
  TOO_MANY_ITERATIONS = 6,
  FAILED_FTOL_TOO_SMALL = 7,
  FAILED_XTOL_TOO_SMALL = 8,
  FAILED_GTOL_TOO_SMALL = 9,
  FAILED_UNKNOWN = 10,
  // next are the masks that are specific to the PK modeling process
  FAILED_NOMATCH = 11, // optimizer failed, but diagnostics string was not recognized
  KTRANS_CLAMPED = 0x10, // = 16 Ktrans was clamped to [0..5]
  VE_CLAMPED = 0x20, // = 32 Ve was clamped to [0..1]
  BAT_DETECTION_FAILED = 0x30, // = 48 BAT detection procedure failed
  BAT_BEFORE_AIF_BAT = 0x40 // = 64 BAT at the voxel was before AIF BAT
};

const std::string OptimizerDiagnosticStrings[] =
{ "failure in leastsquares function",
"lmdif dodgy input",
"converged to ftol",
"converged to xtol",
"converged nicely",
"converged via gtol",
"too many iterations",
"ftol is too small",
"xtol is too small",
"gtol is too small",
"unkown info code"
};

const unsigned NumOptimizerDiagnosticCodes = 11;

namespace itk
{

  class PkModelingCostFunction{
  public:

    enum { SpaceDimension = 2 };
    unsigned int RangeDimension;
    enum ModelType { TOFTS_2_PARAMETER = 1, TOFTS_3_PARAMETER };
    float m_Hematocrit;
    std::string m_IntegrationType;
    int m_ModelType;
    typedef itk::CostFunction::ParametersType ParametersType;

    void SetHematocrit(float hematocrit) {
      m_Hematocrit = hematocrit;
    }

    void SetModelType(int model) {
      m_ModelType = model;
    }

    void SetIntegrationType(std::string ToftsIntegrationMethod){
      m_IntegrationType = ToftsIntegrationMethod;
    }

    void SetNumberOfValues(unsigned int NumberOfValues)
    {
      RangeDimension = NumberOfValues;
    }

    unsigned int GetNumberOfValues(void) const
    {
      return RangeDimension;
    }

    void SetCb(const float* cb, int sz) //BloodConcentrationCurve.
    {
    }

    void SetCv(const float* cv, int sz) //Self signal Y
    {
    }

    void SetTime(const float* cx, int sz) //Self signal X
    {
    }

    void GetValue(const ParametersType & parameters){

    }

    Array <double> GetFittedFunction(const ParametersType & parameters) const{

    }

#define IS_NAN(x) ((x) != (x))

  private:


  };

  class PkModelingOptimizer : public itk::NonLinearOptimizer {
  public:

    void SetCostFunction(itk::PkModelingCostFunction *){

    }

    vnl_amoeba GetOptimizer(){
  
    }

  };

  class AmoebaCostFunction : public itk::SingleValuedCostFunction, public PkModelingCostFunction
  {
  public:
    typedef AmoebaCostFunction                    Self;
    typedef itk::SingleValuedCostFunction   Superclass;
    typedef itk::SmartPointer<Self>           Pointer;
    typedef itk::SmartPointer<const Self>     ConstPointer;

    itkNewMacro(Self);

    typedef Superclass::ParametersType              ParametersType;
    typedef Superclass::DerivativeType              DerivativeType;
    // MeasureType is not an Array type in the AmoebaOptimizer / SingleValuedCostFunction.
    // At many points in the code below, MeasureType is replaced with Array < double >
    typedef Superclass::MeasureType                 MeasureType, ArrayType;
    typedef Superclass::ParametersValueType         ValueType;

    AmoebaCostFunction()
    {
    }

    void SetCb(const float* cb, int sz) //BloodConcentrationCurve.
    {
      Cb.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Cb[i] = cb[i];
      std::cout << "Cb at Simplex Algorithm... " << Cb << std::endl;
    }


    void SetCv(const float* cv, int sz) //Self signal Y
    {
      Cv.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Cv[i] = cv[i];
      //std::cout << "Cv: " << Cv << std::endl;
    }

    void SetTime(const float* cx, int sz) //Self signal X
    {
      Time.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Time[i] = cx[i];
      //std::cout << "Time: " << Time << std::endl;
    }

    unsigned int GetNumberOfParameters(void) const
    {
      if (m_ModelType == TOFTS_2_PARAMETER)
      {
        return 2;
      }
      else // if(m_ModelType == TOFTS_3_PARAMETER)
      {
        return 3;
      }
    }

    MeasureType GetValue(const ParametersType & parameters) const
    {
      MeasureType final_cost = 0;
      Array <double> measure(RangeDimension);

      ValueType Ktrans = parameters[0];
      ValueType Ve = parameters[1];

      // Catch NaN values, replace with a random value to restart fitting. Unsure of
      // robustness in different algorithms - a stopgap measure.
      if (IS_NAN(Ktrans)){
        Ktrans = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }
      if (IS_NAN(Ve)){
        Ve = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }

      Array <double> VeTerm;
      VeTerm = -Ktrans / Ve*Time;
      ValueType deltaT = Time(1) - Time(0);

      ValueType log_e = (-Ktrans / Ve)*deltaT;
      ValueType capital_E = exp(log_e);
      ValueType log_e_2 = pow(log_e, 2);
      ValueType block_A = capital_E - log_e - 1;
      ValueType block_B = capital_E - (capital_E * log_e) - 1;
      ValueType block_ktrans = Ktrans * deltaT / log_e_2;

      if (m_ModelType == TOFTS_3_PARAMETER)
      {
        ValueType f_pv = parameters[2];
        measure = Cv - (1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)) + f_pv*Cb));
        for (unsigned int t = 1; t < RangeDimension; ++t)
        {
          final_cost = final_cost + pow(measure[t], 2);
        }
      }
      else if (m_ModelType == TOFTS_2_PARAMETER)
      {

        if (m_IntegrationType == "Convolutional")
        {
          measure = Cv - (1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm))));
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            final_cost = final_cost + pow(measure[t], 2);
          }
        }

        else if (m_IntegrationType == "Recursive")
        {
          measure[0] = 0;
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            measure[t] = measure[t - 1] * capital_E + (1 / (1.0 - m_Hematocrit)) * block_ktrans * (Cb[t] * block_A - Cb[t - 1] * block_B);
            final_cost = final_cost + pow(Cv[t] - measure[t], 2);
          }
        }
      }
      return final_cost;
    }

    // Note that type change from MeasureType to Array <double>. SingleValuedNonLinearOptimizer
    // does not have array versions of MeasureType.
    Array <double> GetFittedFunction(const ParametersType & parameters) const
    {
      Array <double> measure(RangeDimension);

      ValueType Ktrans = parameters[0];
      ValueType Ve = parameters[1];

      // Attempt to suppress NaN values and replace with random values. Not sure if this
      // function is directly implicated in fitting, in which case these lines may be
      // irrelevant.
      if (IS_NAN(Ktrans)){
        Ktrans = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }
      if (IS_NAN(Ve)){
        Ve = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }

      Array <double> VeTerm;
      VeTerm = -Ktrans / Ve*Time;
      ValueType deltaT = Time(1) - Time(0);

      ValueType log_e = (-Ktrans / Ve)*deltaT;
      ValueType capital_E = exp(log_e);
      ValueType log_e_2 = pow(log_e, 2);
      ValueType block_A = capital_E - log_e - 1;
      ValueType block_B = capital_E - (capital_E * log_e) - 1;
      ValueType block_ktrans = Ktrans * deltaT / log_e_2;

      if (m_ModelType == TOFTS_3_PARAMETER)
      {
        ValueType f_pv = parameters[2];
        measure = 1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)) + f_pv*Cb);
      }
      else if (m_ModelType == TOFTS_2_PARAMETER)
      {

        if (m_IntegrationType == "Convolutional")
        {
          measure = 1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)));
        }
        // New Integration Method
        else if (m_IntegrationType == "Recursive")
        {
          measure[0] = 0;
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            measure[t] = (measure[t - 1] * capital_E + (1 / (1.0 - m_Hematocrit)) * block_ktrans * (Cb[t] * block_A - Cb[t - 1] * block_B));
          }
        }

      }

      return measure;
    }

    //Not going to be used - we have no derivative for the Tofts model at present.
    void GetDerivative(const ParametersType & /* parameters*/,
      DerivativeType  & /*derivative*/) const
    {
    }

  protected:
    virtual ~AmoebaCostFunction(){}
  private:

    Array <double> Cv, Cb, Time;

    Array <double> Convolution(Array <double> X, Array <double> Y) const
    {
      Array <double> Z;
      Z = vnl_convolve(X, Y).extract(X.size(), 0);
      return Z;
    };

    Array <double> Exponential(Array <double> X) const
    {
      Array <double> Z;
      Z.set_size(X.size());
      for (unsigned int i = 0; i < X.size(); i++)
      {
        Z[i] = exp(X(i));
      }
      return Z;
    };

    int constraintFunc(ValueType x) const
    {
      if (x < 0 || x>1)
        return 1;
      else
        return 0;

    };


  };

  class LMCostFunction : public itk::MultipleValuedCostFunction, public PkModelingCostFunction
  {
  public:
    typedef LMCostFunction                    Self;
    typedef itk::MultipleValuedCostFunction   Superclass;
    typedef itk::SmartPointer<Self>           Pointer;
    typedef itk::SmartPointer<const Self>     ConstPointer;

    itkNewMacro(Self);

    typedef Superclass::ParametersType              ParametersType;
    typedef Superclass::DerivativeType              DerivativeType;
    typedef Superclass::MeasureType                 MeasureType, ArrayType;
    typedef Superclass::ParametersValueType         ValueType;

    LMCostFunction()
    {
    }

    unsigned int GetNumberOfValues(void) const
    {
      return RangeDimension;
    }

    void SetCb(const float* cb, int sz) //BloodConcentrationCurve.
    {
      Cb.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Cb[i] = cb[i];
      std::cout << "Cb at LM Algorithm..." << Cb << std::endl;
    }


    void SetCv(const float* cv, int sz) //Self signal Y
    {
      Cv.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Cv[i] = cv[i];
      //std::cout << "Cv: " << Cv << std::endl;
    }

    void SetTime(const float* cx, int sz) //Self signal X
    {
      Time.set_size(sz);
      for (int i = 0; i < sz; ++i)
        Time[i] = cx[i];
      //std::cout << "Time: " << Time << std::endl;
    }

    MeasureType GetValue(const ParametersType & parameters) const
    {
      MeasureType final_cost(RangeDimension);
      MeasureType measure(RangeDimension);

      ValueType Ktrans = parameters[0];
      ValueType Ve = parameters[1];

      // Catch NaN values, replace with a random value to restart fitting. Unsure of
      // robustness in different algorithms - a stopgap measure.
      if (IS_NAN(Ktrans)){
        Ktrans = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }
      if (IS_NAN(Ve)){
        Ve = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }

      MeasureType VeTerm;
      VeTerm = -Ktrans / Ve*Time;
      ValueType deltaT = Time(1) - Time(0);

      ValueType log_e = (-Ktrans / Ve)*deltaT;
      ValueType capital_E = exp(log_e);
      ValueType log_e_2 = pow(log_e, 2);
      ValueType block_A = capital_E - log_e - 1;
      ValueType block_B = capital_E - (capital_E * log_e) - 1;
      ValueType block_ktrans = Ktrans * deltaT / log_e_2;

      final_cost[0] = 0;
      if (m_ModelType == TOFTS_3_PARAMETER)
      {
        ValueType f_pv = parameters[2];
        measure = Cv - (1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)) + f_pv*Cb));
        for (unsigned int t = 1; t < RangeDimension; ++t)
        {
          final_cost = final_cost + pow(measure[t], 2);
        }
      }
      else if (m_ModelType == TOFTS_2_PARAMETER)
      {

        if (m_IntegrationType == "Convolutional")
        {
          measure = Cv - (1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm))));
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            final_cost[t] = pow(measure[t], 2);
          }
        }

        else if (m_IntegrationType == "Recursive")
        {
          measure[0] = 0;
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            measure[t] = measure[t - 1] * capital_E + (1 / (1.0 - m_Hematocrit)) * block_ktrans * (Cb[t] * block_A - Cb[t - 1] * block_B);
            final_cost[t] = pow(Cv[t] - measure[t], 2);
          }
        }
      }
      return final_cost;
    }

    MeasureType GetFittedFunction(const ParametersType & parameters) const
    {
      MeasureType measure(RangeDimension);

      ValueType Ktrans = parameters[0];
      ValueType Ve = parameters[1];

      // Attempt to suppress NaN values and replace with random values. Not sure if this
      // function is directly implicated in fitting, in which case these lines may be
      // irrelevant.
      if (IS_NAN(Ktrans)){
        Ktrans = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }
      if (IS_NAN(Ve)){
        Ve = .01 + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / (.5 - .01)));
      }

      MeasureType VeTerm;
      VeTerm = -Ktrans / Ve*Time;
      ValueType deltaT = Time(1) - Time(0);

      ValueType log_e = (-Ktrans / Ve)*deltaT;
      ValueType capital_E = exp(log_e);
      ValueType log_e_2 = pow(log_e, 2);
      ValueType block_A = capital_E - log_e - 1;
      ValueType block_B = capital_E - (capital_E * log_e) - 1;
      ValueType block_ktrans = Ktrans * deltaT / log_e_2;

      if (m_ModelType == TOFTS_3_PARAMETER)
      {
        ValueType f_pv = parameters[2];
        measure = 1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)) + f_pv*Cb);
      }
      else if (m_ModelType == TOFTS_2_PARAMETER)
      {

        if (m_IntegrationType == "Convolutional")
        {
          measure = 1 / (1.0 - m_Hematocrit)*(Ktrans*deltaT*Convolution(Cb, Exponential(VeTerm)));
        }
        // New Integration Method
        else if (m_IntegrationType == "Recursive")
        {
          measure[0] = 0;
          for (unsigned int t = 1; t < RangeDimension; ++t)
          {
            measure[t] = (measure[t - 1] * capital_E + (1 / (1.0 - m_Hematocrit)) * block_ktrans * (Cb[t] * block_A - Cb[t - 1] * block_B));
          }
        }

      }
    }

    //Not going to be used
    void GetDerivative(const ParametersType & /* parameters*/,
      DerivativeType  & /*derivative*/) const
    {
    }

    unsigned int GetNumberOfParameters(void) const
    {
      if (m_ModelType == TOFTS_2_PARAMETER)
      {
        return 2;
      }
      else // if(m_ModelType == TOFTS_3_PARAMETER)
      {
        return 3;
      }
    }

  protected:
    virtual ~LMCostFunction(){}
  private:

    ArrayType Cv, Cb, Time;

    ArrayType Convolution(ArrayType X, ArrayType Y) const
    {
      ArrayType Z;
      Z = vnl_convolve(X, Y).extract(X.size(), 0);
      return Z;
    };

    ArrayType Exponential(ArrayType X) const
    {
      ArrayType Z;
      Z.set_size(X.size());
      for (unsigned int i = 0; i < X.size(); i++)
      {
        Z[i] = exp(X(i));
      }
      return Z;
    };

    int constraintFunc(ValueType x) const
    {
      if (x < 0 || x>1)
        return 1;
      else
        return 0;

    };

  };

  class CommandIterationUpdateAmoeba : public itk::Command
  {
  public:
    typedef  CommandIterationUpdateAmoeba   Self;
    typedef  itk::Command                               Superclass;
    typedef itk::SmartPointer<Self>                     Pointer;
    itkNewMacro(Self);
  protected:
    CommandIterationUpdateAmoeba()
    {
      m_IterationNumber = 0;
    }
    virtual ~CommandIterationUpdateAmoeba(){}
  public:
    typedef itk::AmoebaOptimizer   OptimizerType;
    typedef   const OptimizerType   *          OptimizerPointer;

    void Execute(itk::Object *caller, const itk::EventObject & event)
    {
      Execute((const itk::Object *)caller, event);
    }

    void Execute(const itk::Object * object, const itk::EventObject & event)
    {
      //std::cout << m_IterationNumber++ << std::endl;
      //std::cout << "Observer::Execute() " << std::endl;
      OptimizerPointer optimizer =
        dynamic_cast<OptimizerPointer>(object);
      if (m_FunctionEvent.CheckEvent(&event))
      {
        //std::cout << m_IterationNumber++ << "   ";
        //std::cout << optimizer->GetCachedValue() << "   ";
        //std::cout << optimizer->GetCachedCurrentPosition() << std::endl;
      }
      else if (m_GradientEvent.CheckEvent(&event))
      {
        //std::cout << "Gradient " << optimizer->GetCachedDerivative() << "   ";
      }

    }
  private:
    unsigned long m_IterationNumber;

    itk::FunctionEvaluationIterationEvent m_FunctionEvent;
    itk::GradientEvaluationIterationEvent m_GradientEvent;
  };

  class CommandIterationUpdateLevenbergMarquardt : public itk::Command
  {
  public:
    typedef  CommandIterationUpdateLevenbergMarquardt   Self;
    typedef  itk::Command                               Superclass;
    typedef itk::SmartPointer<Self>                     Pointer;
    itkNewMacro(Self);
  protected:
    CommandIterationUpdateLevenbergMarquardt()
    {
      m_IterationNumber = 0;
    }
    virtual ~CommandIterationUpdateLevenbergMarquardt(){}
  public:
    typedef itk::LevenbergMarquardtOptimizer   OptimizerType;
    typedef   const OptimizerType   *          OptimizerPointer;

    void Execute(itk::Object *caller, const itk::EventObject & event)
    {
      Execute((const itk::Object *)caller, event);
    }

    void Execute(const itk::Object * object, const itk::EventObject & event)
    {
      //std::cout << "Observer::Execute() " << std::endl;
      OptimizerPointer optimizer =
        dynamic_cast<OptimizerPointer>(object);
      if (m_FunctionEvent.CheckEvent(&event))
      {
        // std::cout << m_IterationNumber++ << "   ";
        // std::cout << optimizer->GetCachedValue() << "   ";
        // std::cout << optimizer->GetCachedCurrentPosition() << std::endl;
      }
      else if (m_GradientEvent.CheckEvent(&event))
      {
        std::cout << "Gradient " << optimizer->GetCachedDerivative() << "   ";
      }

    }
  private:
    unsigned long m_IterationNumber;

    itk::FunctionEvaluationIterationEvent m_FunctionEvent;
    itk::GradientEvaluationIterationEvent m_GradientEvent;
  };

  bool pk_solver(int signalSize,
    const float* timeAxis,
    const float* PixelConcentrationCurve,
    const float* BloodConcentrationCurve,
    const std::string ToftsIntegrationMethod,
    const std::string FittingMethod,
    float& Ktrans,
    float& Ve,
    float& Fpv,
    float fTol = 1e-4f,
    float gTol = 1e-4f,
    float xTol = 1e-5f,
    float epsilon = 1e-9f,
    int maxIter = 200,
    float hematocrit = 0.4f,
    int modelType = itk::AmoebaCostFunction::TOFTS_2_PARAMETER,
    int constantBAT = 0,
    const std::string BATCalculationMode = "PeakGradient");

  // returns diagnostic error code from the VNL optimizer,
  //  as defined by OptimizerDiagnosticCodes, and masked to indicate
  //  wheather Ktrans or Ve were clamped.
  unsigned pk_solver(int signalSize,
    const float* timeAxis,
    const float* PixelConcentrationCurve,
    const float* BloodConcentrationCurve,
    const std::string ToftsIntegrationMethod,
    float& Ktrans,
    float& Ve,
    float& Fpv,
    float fTol,
    float gTol,
    float xTol,
    float epsilon,
    int maxIter,
    float hematocrit,
    const std::string FittingMethod,
    itk::PkModelingOptimizer* optimizer,
    itk::PkModelingCostFunction* costFunction,
    int modelType = itk::PkModelingCostFunction::TOFTS_2_PARAMETER,
    int constantBAT = 0,
    const std::string BATCalculationMode = "PeakGradient");

  void pk_report();
  void pk_clear();

  bool convert_signal_to_concentration(unsigned int signalSize,
    const float* SignalIntensityCurve,
    float T1, float TR, float FA,
    float* concentration,
    float relaxivity = 4.9E-3f,
    float s0 = -1.0f,
    float S0GradThresh = 15.0f);

  float area_under_curve(int signalSize, const float* timeAxis, const float* concentration, int BATIndex, float aucTimeInterval);

  float intergrate(float* yValues, float * xValues, int size);

  void compute_derivative(int signalSize, const float* SingnalY, float* YDeriv);

  void compute_derivative_forward(int signalSize, const float* SignalY, float* YDeriv);

  void compute_derivative_backward(int signalSize, const float* SignalY, float* YDeriv);

  float get_signal_max(int signalSize, const float* SignalY, int& index);

  bool compute_bolus_arrival_time(int signalSize, const float* SignalY,
    int& ArrivalTime, int& FirstPeak, float& MaxSlope);

  void compute_gradient(int signalSize, const float* SignalY, float* SignalGradient);

  void compute_gradient_forward(int signalSize, const float* SignalY, float* SignalGradient);

  void compute_gradient_backward(int signalSize, const float* SignalY, float* SignalGradient);

  float compute_s0_using_sumsignal_properties(int signalSize, const float* SignalY,
    const short* lowGradIndex, int FirstPeak);

  float compute_s0_individual_curve(int signalSize, const float* SignalY, float S0GradThresh, std::string BATCalculationMode, int constantBAT);

};
#endif
