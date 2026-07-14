#include "ikaros.h"

#include <algorithm>
using namespace ikaros;

class KalmanFilter: public Module
{
    parameter processNoise;
    parameter observationNoise;
    parameter inputSize;
    parameter innovationGate;
    parameter inversionJitter;
    parameter aParameter;
    parameter bParameter;
    parameter hParameter;
    parameter qParameter;
    parameter rParameter;
    parameter initialStateParameter;
    parameter initialPParameter;

    matrix input;
    matrix inputValid;
    matrix observation;
    matrix observationValid;
    matrix reset;
    matrix state;
    matrix innovation;
    matrix kalmanGain;
    matrix gated;
    matrix normalizedInnovation;

    matrix A;
    matrix B;
    matrix H;
    matrix P;
    matrix Q;
    matrix R;
    matrix initialState;
    matrix initialP;

    matrix AT;
    matrix HT;
    matrix I;

    matrix predictedState;
    matrix predictedCovariance;
    matrix tmpState;
    matrix tmpStateMatrix;
    matrix tmpStateMatrix2;
    matrix tmpObservation;
    matrix tmpStateObservation;
    matrix tmpObservationState;
    matrix residualCovariance;
    matrix residualCovarianceInverse;
    matrix jitteredResidualCovariance;
    matrix kalmanGainTranspose;
    matrix innovationCovariance;
    matrix innovationCovarianceTranspose;
    matrix covarianceCorrection;
    matrix covarianceNoise;
    matrix kalmanTimesObservationNoise;

    bool scalarProcessNoise = false;
    bool scalarObservationNoise = false;

    void Init()
    {
        Bind(processNoise, "process_noise");
        Bind(observationNoise, "observation_noise");
        Bind(inputSize, "input_size");
        Bind(innovationGate, "innovation_gate");
        Bind(inversionJitter, "inversion_jitter");
        Bind(aParameter, "A");
        Bind(bParameter, "B");
        Bind(hParameter, "H");
        Bind(qParameter, "Q");
        Bind(rParameter, "R");
        Bind(initialStateParameter, "INITIAL_STATE");
        Bind(initialPParameter, "INITIAL_P");

        Bind(input, "INPUT");
        Bind(inputValid, "INPUT_VALID");
        Bind(observation, "OBSERVATION");
        Bind(observationValid, "OBSERVATION_VALID");
        Bind(reset, "RESET");
        Bind(state, "STATE");
        Bind(innovation, "INNOVATION");
        Bind(kalmanGain, "KALMAN_GAIN");
        Bind(gated, "GATED");
        Bind(normalizedInnovation, "NORMALIZED_INNOVATION");

        Bind(P, "P");

        RefreshConfigurationMatrices(true);
        ValidateShapes();
        ValidateParameters();
        AllocateBuffers();
        InitializeMatrixDefaults();
    }

    void Tick()
    {
        if(ShouldReset())
        {
            ResetFilter();
            return;
        }

        RefreshConfigurationMatrices(false);
        A.transpose(AT);
        H.transpose(HT);
        UpdateNoiseMatrices();

        predictedState.matvec(A, state);
        if(input.connected() && SignalActive(inputValid))
        {
            tmpState.matvec(B, input);
            predictedState.add(tmpState);
        }

        tmpStateMatrix.matmul(A, P);
        predictedCovariance.matmul(tmpStateMatrix, AT);
        predictedCovariance.add(Q);

        if(!SignalActive(observationValid))
        {
            PredictionOnly();
            return;
        }

        tmpObservation.matvec(H, predictedState);
        innovation.subtract(observation, tmpObservation);

        tmpObservationState.matmul(H, predictedCovariance);
        residualCovariance.matmul(tmpObservationState, HT);
        residualCovariance.add(R);

        if(!InvertResidualCovariance())
        {
            return;
        }

        if(ObservationRejected())
        {
            state.copy(predictedState);
            P.copy(predictedCovariance);
            kalmanGain.set(0.0f);
            gated(0) = 1.0f;
            return;
        }

        gated(0) = 0.0f;
        tmpStateObservation.matmul(predictedCovariance, HT);
        kalmanGain.matmul(tmpStateObservation, residualCovarianceInverse);

        tmpState.matvec(kalmanGain, innovation);
        state.add(predictedState, tmpState);

        UpdateCovarianceJoseph();
    }

    void ValidateShapes()
    {
        if(input.connected() && input.rank() != 1)
            throw exception("KalmanFilter: INPUT must be a vector when connected.", path_);

        if(inputValid.connected() && inputValid.rank() != 1)
            throw exception("KalmanFilter: INPUT_VALID must be a vector when connected.", path_);

        if(observationValid.connected() && observationValid.rank() != 1)
            throw exception("KalmanFilter: OBSERVATION_VALID must be a vector when connected.", path_);

        if(observation.rank() != 1 || state.rank() != 1 || innovation.rank() != 1 || initialState.rank() != 1)
            throw exception("KalmanFilter: OBSERVATION, STATE, INNOVATION, and INITIAL_STATE must be vectors.", path_);

        const int n = state.size();
        const int k = observation.size();

        if(innovation.size() != k)
            throw exception("KalmanFilter: INNOVATION size must match OBSERVATION size.", path_);

        if(A.rank() != 2 || A.rows() != n || A.cols() != n)
            throw exception("KalmanFilter: A must have shape [STATE.size, STATE.size].", path_);

        if(B.rank() != 2 || B.rows() != n)
            throw exception("KalmanFilter: B must have shape [STATE.size, input_size].", path_);

        if(input.connected() && B.cols() != input.size())
            throw exception("KalmanFilter: B columns must match INPUT size.", path_);

        if(H.rank() != 2 || H.rows() != k || H.cols() != n)
            throw exception("KalmanFilter: H must have shape [OBSERVATION.size, STATE.size].", path_);

        if(P.rank() != 2 || P.rows() != n || P.cols() != n)
            throw exception("KalmanFilter: P must have shape [STATE.size, STATE.size].", path_);

        if(Q.rank() != 2 || Q.rows() != n || Q.cols() != n)
            throw exception("KalmanFilter: Q must have shape [STATE.size, STATE.size].", path_);

        if(R.rank() != 2 || R.rows() != k || R.cols() != k)
            throw exception("KalmanFilter: R must have shape [OBSERVATION.size, OBSERVATION.size].", path_);

        if(initialState.size() != n)
            throw exception("KalmanFilter: INITIAL_STATE size must match STATE size.", path_);

        if(initialP.rank() != 2 || initialP.rows() != n || initialP.cols() != n)
            throw exception("KalmanFilter: INITIAL_P must have shape [STATE.size, STATE.size].", path_);

        if(kalmanGain.rank() != 2 || kalmanGain.rows() != n || kalmanGain.cols() != k)
            throw exception("KalmanFilter: KALMAN_GAIN must have shape [STATE.size, OBSERVATION.size].", path_);

        if(gated.rank() != 1 || gated.size() != 1)
            throw exception("KalmanFilter: GATED must have size 1.", path_);

        if(normalizedInnovation.rank() != 1 || normalizedInnovation.size() != 1)
            throw exception("KalmanFilter: NORMALIZED_INNOVATION must have size 1.", path_);

        if(reset.connected() && reset.rank() != 1)
            throw exception("KalmanFilter: RESET must be a vector when connected.", path_);
    }

    void ValidateParameters()
    {
        if(processNoise.as_float() < 0.0f)
            throw exception("KalmanFilter: process_noise must be non-negative.", path_);

        if(observationNoise.as_float() < 0.0f)
            throw exception("KalmanFilter: observation_noise must be non-negative.", path_);

        if(inputSize.as_int() < 1)
            throw exception("KalmanFilter: input_size must be at least 1.", path_);

        if(innovationGate.as_float() < 0.0f)
            throw exception("KalmanFilter: innovation_gate must be non-negative.", path_);

        if(inversionJitter.as_float() < 0.0f)
            throw exception("KalmanFilter: inversion_jitter must be non-negative.", path_);
    }

    void AllocateBuffers()
    {
        const int n = state.size();
        const int k = observation.size();

        AT.realloc(n, n);
        HT.realloc(n, k);
        I.realloc(n, n);

        predictedState.realloc(n);
        predictedCovariance.realloc(n, n);
        tmpState.realloc(n);
        tmpStateMatrix.realloc(n, n);
        tmpStateMatrix2.realloc(n, n);
        tmpObservation.realloc(k);
        tmpStateObservation.realloc(n, k);
        tmpObservationState.realloc(k, n);
        residualCovariance.realloc(k, k);
        residualCovarianceInverse.realloc(k, k);
        jitteredResidualCovariance.realloc(k, k);
        kalmanGainTranspose.realloc(k, n);
        innovationCovariance.realloc(n, n);
        innovationCovarianceTranspose.realloc(n, n);
        covarianceCorrection.realloc(n, n);
        covarianceNoise.realloc(n, n);
        kalmanTimesObservationNoise.realloc(n, k);
    }

    void InitializeMatrixDefaults()
    {
        MakeIdentity(I, 1.0f);

        if(IsZero(A))
            MakeIdentity(A, 1.0f);

        if(IsZero(H))
            MakeIdentity(H, 1.0f);

        if(IsZero(P))
        {
            if(IsZero(initialP))
                MakeIdentity(P, 1.0f);
            else
                P.copy(initialP);
        }

        A.transpose(AT);
        H.transpose(HT);
        UpdateNoiseMatrices();

        if(IsZero(initialP))
            initialP.copy(P);
    }

    void UpdateNoiseMatrices()
    {
        if(scalarProcessNoise)
            MakeIdentity(Q, processNoise.as_float());

        if(scalarObservationNoise)
            MakeIdentity(R, observationNoise.as_float());
    }

    void RefreshConfigurationMatrices(bool includeInitialValues)
    {
        const int n = state.size();
        const int k = observation.size();
        const int m = input.connected() ? input.size() : inputSize.as_int();

        CopyMatrixParameter(A, aParameter, n, n);
        CopyMatrixParameter(B, bParameter, n, m);
        CopyMatrixParameter(H, hParameter, k, n);
        CopyMatrixParameter(Q, qParameter, n, n);
        CopyMatrixParameter(R, rParameter, k, k);

        if(includeInitialValues)
        {
            CopyVectorParameter(initialState, initialStateParameter, n);
            CopyMatrixParameter(initialP, initialPParameter, n, n);
        }

        scalarProcessNoise = IsZero(Q);
        scalarObservationNoise = IsZero(R);
    }

    void CopyMatrixParameter(matrix & target, const parameter & source, int rows, int cols)
    {
        matrix value;
        value.copy(source);

        if(IsZeroScalar(value))
        {
            target.realloc(rows, cols);
            target.set(0.0f);
            return;
        }

        target.copy(value);
    }

    void CopyVectorParameter(matrix & target, const parameter & source, int size)
    {
        matrix value;
        value.copy(source);

        if(IsZeroScalar(value))
        {
            target.realloc(size);
            target.set(0.0f);
            return;
        }

        target.copy(value);
    }

    bool IsZeroScalar(const matrix & m) const
    {
        return m.size() == 1 && m.data()[0] == 0.0f;
    }

    bool ShouldReset() const
    {
        if(!reset.connected())
            return false;

        for(int i = 0; i < reset.size(); ++i)
            if(reset.data()[i] != 0.0f)
                return true;

        return false;
    }

    void ResetFilter()
    {
        state.copy(initialState);
        P.copy(initialP);
        innovation.set(0.0f);
        kalmanGain.set(0.0f);
        gated(0) = 0.0f;
        normalizedInnovation(0) = 0.0f;
    }

    bool SignalActive(const matrix & signal) const
    {
        if(!signal.connected())
            return true;

        for(int i = 0; i < signal.size(); ++i)
            if(signal.data()[i] != 0.0f)
                return true;

        return false;
    }

    void PredictionOnly()
    {
        state.copy(predictedState);
        P.copy(predictedCovariance);
        innovation.set(0.0f);
        kalmanGain.set(0.0f);
        gated(0) = 0.0f;
        normalizedInnovation(0) = 0.0f;
    }

    bool ObservationRejected()
    {
        tmpObservation.matvec(residualCovarianceInverse, innovation);

        normalizedInnovation(0) = 0.0f;
        for(int i = 0; i < innovation.size(); ++i)
            normalizedInnovation(0) += innovation(i) * tmpObservation(i);

        const float gate = innovationGate.as_float();
        if(gate <= 0.0f)
            return false;

        return normalizedInnovation(0) > gate;
    }

    bool InvertResidualCovariance()
    {
        try
        {
            residualCovarianceInverse.inv(residualCovariance);
            return true;
        }
        catch(const std::exception & e)
        {
            if(inversionJitter.as_float() <= 0.0f)
            {
                Warning(std::string("KalmanFilter residual covariance is singular: ") + e.what());
                return false;
            }
        }

        jitteredResidualCovariance.copy(residualCovariance);
        AddDiagonal(jitteredResidualCovariance, inversionJitter.as_float());

        try
        {
            residualCovarianceInverse.inv(jitteredResidualCovariance);
            return true;
        }
        catch(const std::exception & e)
        {
            Warning(std::string("KalmanFilter residual covariance is singular after jitter: ") + e.what());
            return false;
        }
    }

    void AddDiagonal(matrix & m, float value)
    {
        const int diagonalCount = std::min(m.rows(), m.cols());
        for(int i = 0; i < diagonalCount; ++i)
            m(i, i) += value;
    }

    void UpdateCovarianceJoseph()
    {
        tmpStateMatrix2.matmul(kalmanGain, H);
        innovationCovariance.subtract(I, tmpStateMatrix2);
        innovationCovariance.transpose(innovationCovarianceTranspose);

        tmpStateMatrix.matmul(innovationCovariance, predictedCovariance);
        covarianceCorrection.matmul(tmpStateMatrix, innovationCovarianceTranspose);

        kalmanGain.transpose(kalmanGainTranspose);
        kalmanTimesObservationNoise.matmul(kalmanGain, R);
        covarianceNoise.matmul(kalmanTimesObservationNoise, kalmanGainTranspose);

        P.add(covarianceCorrection, covarianceNoise);
        Symmetrize(P);
    }

    void MakeIdentity(matrix & m, float diagonal)
    {
        m.set(0.0f);
        const int diagonalCount = std::min(m.rows(), m.cols());
        for(int i = 0; i < diagonalCount; ++i)
            m(i, i) = diagonal;
    }

    bool IsZero(const matrix & m)
    {
        for(int i = 0; i < m.size(); ++i)
            if(m.data()[i] != 0.0f)
                return false;
        return true;
    }

    void Symmetrize(matrix & m)
    {
        for(int row = 0; row < m.rows(); ++row)
        {
            for(int col = row + 1; col < m.cols(); ++col)
            {
                const float value = 0.5f * (m(row, col) + m(col, row));
                m(row, col) = value;
                m(col, row) = value;
            }
        }
    }

};

INSTALL_CLASS(KalmanFilter)
