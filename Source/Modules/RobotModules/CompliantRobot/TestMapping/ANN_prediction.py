import numpy as np
import mmap
import os
import json
from time import sleep
import sys
try:
    import posix_ipc
except ImportError as e:
    print(f"Error importing posix_ipc: {e}", file=sys.stderr)
    print(f"Current environment: {os.environ.get('VIRTUAL_ENV')}", file=sys.stderr)
    sys.exit(1)
from keras.models import Sequential, load_model
from keras.layers import Dense, BatchNormalization, Dropout, Input
from keras.regularizers import l2
from keras.optimizers import Adam

# Update the shared memory path for macOS - remove /private/tmp/
SHM_NAME = "/ikaros_ann_shm"  # Changed this back
FLOAT_COUNT = 12  # 10 inputs + 2 predictions (tilt and pan)
MEM_SIZE = (FLOAT_COUNT * 4) + 1  # 4 bytes per float + 1 byte for bool

class SharedMemory:
    def __init__(self):
        try:
            print(f"Attempting to open shared memory at {SHM_NAME}", file=sys.stderr)
            # Use posix_ipc to open shared memory
            self.memory = posix_ipc.SharedMemory(SHM_NAME)
            self.size = MEM_SIZE  # Use the same size calculation as C++
            
            # Map the shared memory
            self.shared_data = mmap.mmap(self.memory.fd, self.size)
            print(f"Successfully mapped shared memory of size {self.size} bytes", 
                  file=sys.stderr)
                
        except Exception as e:
            print(f"Error initializing shared memory: {e}", file=sys.stderr)
            print(f"Current working directory: {os.getcwd()}", file=sys.stderr)
            raise
            
    def __del__(self):
        try:
            if hasattr(self, 'shared_data'):
                self.shared_data.close()
            if hasattr(self, 'memory'):
                self.memory.close_fd()
        except Exception as e:
            print(f"Error cleaning up shared memory: {e}", file=sys.stderr)
    
    def read_data(self):
        self.shared_data.seek(0)
        data = np.frombuffer(self.shared_data.read(12 * 4), dtype=np.float32)
        # First 2 are positions (tilt, pan)
        # Next 3 are gyro
        # Next 3 are accel
        # Next 2 are distances to goal
        # Last 2 are for predictions (tilt, pan)
        return {
            'positions': data[:2],
            'gyro': data[2:5],
            'accel': data[5:8],
            'distances': data[8:10],
            'predictions': data[10:]
        }
    
    def write_prediction(self, predictions):
        # Write predictions starting at the correct offset
        self.shared_data.seek(10 * 4)  # Skip past input data
        self.shared_data.write(np.array(predictions, dtype=np.float32).tobytes())
        # Clear new_data flag
        self.shared_data.seek(12 * 4)  # Position for flag
        self.shared_data.write(bytes([0]))
    
    def get_new_data_flag(self):
        self.shared_data.seek(12 * 4)  # Position for flag
        return bool(ord(self.shared_data.read(1)))

def normalise_input(x, means_stds):
    # Input x should be a numpy array with shape (1, 10)
    normalized = np.zeros((1, 10))  # Create array with correct shape
    normalized[0,0] = (x[0] - means_stds['TiltPosition']['mean']) / means_stds['TiltPosition']['std']
    normalized[0,1] = (x[1] - means_stds['PanPosition']['mean']) / means_stds['PanPosition']['std']
    normalized[0,2] = (x[2] - means_stds['GyroX']['mean']) / means_stds['GyroX']['std']
    normalized[0,3] = (x[3] - means_stds['GyroY']['mean']) / means_stds['GyroY']['std']
    normalized[0,4] = (x[4] - means_stds['GyroZ']['mean']) / means_stds['GyroZ']['std']
    normalized[0,5] = (x[5] - means_stds['AccelX']['mean']) / means_stds['AccelX']['std']
    normalized[0,6] = (x[6] - means_stds['AccelY']['mean']) / means_stds['AccelY']['std']
    normalized[0,7] = (x[7] - means_stds['AccelZ']['mean']) / means_stds['AccelZ']['std']
    normalized[0,8] = (x[8] - means_stds['tilt_distance']['mean']) / means_stds['tilt_distance']['std']
    normalized[0,9] = (x[9] - means_stds['pan_distance']['mean']) / means_stds['pan_distance']['std']
    return normalized

def create_model_with_weights(weights_path, model_name):    
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
        model.compile(optimizer=Adam(learning_rate=0.001), loss='mean_squared_error', metrics=['mae'])

        model.load_weights(weights_path)
        return model


def main():
    try:
        directory = os.path.dirname(os.path.abspath(__file__))
        
        # Load models and means/stds
        tilt_model = create_model_with_weights(directory + '/weights/tilt.weights.h5', 'tilt_model')
        pan_model = create_model_with_weights(directory + '/weights/pan.weights.h5', 'pan_model')
        
        with open(directory + '/weights/mean_std.json', 'r') as f:
            means_stds = json.load(f)
        
        shm = SharedMemory()  # This will now use the correct SHM_NAME
        print("Successfully connected to shared memory", file=sys.stderr)
        
        while True:
            if shm.get_new_data_flag():
                # Read all input data
                input_data = shm.read_data()
                
                # Create input array for models
                model_inputs = np.concatenate([
                    input_data['positions'],
                    input_data['gyro'],
                    input_data['accel'],
                    input_data['distances']
                ]).reshape(1, -1)
                print(f"Model inputs: {model_inputs}", file=sys.stderr) 
                # Normalize inputs
                normalized = normalise_input(model_inputs[0], means_stds)
                
                # Make predictions
                tilt_pred = tilt_model.predict(normalized, verbose=0)[0]
                pan_pred = pan_model.predict(normalized, verbose=0)[0]
                
                # Denormalize predictions
                tilt_pred = tilt_pred * means_stds['TiltCurrent']['std'] + means_stds['TiltCurrent']['mean']
                pan_pred = pan_pred * means_stds['PanCurrent']['std'] + means_stds['PanCurrent']['mean']
                
                # Write predictions
                predictions = [float(tilt_pred[0]), float(pan_pred[0])]
                shm.write_prediction(predictions)
                print(f"Predictions: {predictions}", file=sys.stderr)
            sleep(0.001)
            
    except Exception as e:
        print(f"Error in main: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()