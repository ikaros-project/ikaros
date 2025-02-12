from keras.models import Sequential, load_model
from keras.layers import Dense, BatchNormalization, Dropout, Input
from keras.regularizers import l2
import sys
import numpy as np
import os

def create_model_with_weights(weights_path, model_name):
    # Load the full model instead of creating new and loading weights
    try:
        model = load_model(weights_path)
        model.name = model_name
        return model
    except Exception as e:
        print(f"Failed to load full model, falling back to weights-only loading: {e}")
        
        # Fallback to your original implementation
        model = Sequential()
        model.name = model_name
        model.add(Input(shape=(10,)))
        model.add(Dense(128, activation='relu', kernel_regularizer=l2(0.001)))
        model.add(BatchNormalization())
        model.add(Dropout(0.3))
        model.add(Dense(32, activation='relu'))
        model.add(BatchNormalization())
        model.add(Dropout(0.3))
        model.add(Dense(1, activation='linear'))
        model.compile(optimizer='adam', loss='mean_squared_error', metrics=['mae'])

        model.load_weights(weights_path)
        return model

# get the directory of the script
directory = os.path.dirname(os.path.abspath(__file__))
print("pythonscript directory: ", directory)

# Load the models once at startup
tilt_model = create_model_with_weights(directory + '/weights/tilt.weights.h5', 'tilt_model')
pan_model = create_model_with_weights(directory + '/weights/pan.weights.h5', 'pan_model')

def predict_tilt(x):
    return tilt_model.predict(x, verbose=0)

def predict_pan(x):
    return pan_model.predict(x, verbose=0)

# Continuous processing loop
while True:
    try:
        # Read exactly 40 bytes (10 float32 values)
        input_data = sys.stdin.buffer.read(40)
        if not input_data or len(input_data) != 40:
            break  # Exit if pipe is closed or invalid data
            
        print("input_data: ", input_data)
        # Convert bytes to numpy array of floats
        x = np.frombuffer(input_data, dtype=np.float32)
        x = x.reshape(1, 10)
        
        # Make predictions
        tilt_prediction = predict_tilt(x)
        pan_prediction = predict_pan(x)
        
        # Write predictions as binary float32 data
        output = np.array([tilt_prediction[0][0], pan_prediction[0][0]], dtype=np.float32)
        sys.stdout.buffer.write(output.tobytes())
        sys.stdout.buffer.flush()

    except Exception as e:
        sys.stderr.write(f"Error: {str(e)}\n")
        sys.stderr.flush()
        break
        
#Close the pipe
sys.stdin.close()
sys.stdout.close()




