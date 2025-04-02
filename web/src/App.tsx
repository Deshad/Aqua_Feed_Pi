import React, { useState, useEffect } from 'react';
import { Power, Droplets, Camera, Fish, WifiOff, AlertCircle, Wifi, Settings } from 'lucide-react';
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend, ResponsiveContainer } from 'recharts';

interface SensorData {
  ph: number;
  fishDetected: boolean;
  motorStatus: boolean;
  current_ph_voltage?: number;
  feed_count?: number;
  auto_feed_count?: number;
  last_feed_time?: string;
  ph_sensor_initialized?: boolean;
}

interface ConnectionStatus {
  isConnected: boolean;
  lastAttempt: Date | null;
  retryCount: number;
}

interface PhHistoryItem {
  time: Date;
  value: number;
}

function App() {
  const [sensorData, setSensorData] = useState<SensorData>({
    ph: 7.0,
    fishDetected: false,
    motorStatus: false
  });
  const [isAutoMode, setIsAutoMode] = useState(true);
  const [connectionError, setConnectionError] = useState<string | null>(null);
  const [connectionStatus, setConnectionStatus] = useState<ConnectionStatus>({
    isConnected: false,
    lastAttempt: null,
    retryCount: 0
  });
  const [phHistory, setPhHistory] = useState<PhHistoryItem[]>([]);

  const MAX_RETRIES = 3;
  const RETRY_DELAY = 2000;
  
  const toggleMotor = async () => {
    try {
      setConnectionError(null);
      const response = await fetch('/api/feed_fish', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ 
          command: "feed_fish",
          override: true
        }),
      });
      
      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
      
      const data = await response.json();
      if (!data.success) throw new Error(data.message || 'Motor control failed');
      
      await fetchSensorData();
      setConnectionStatus(prev => ({ ...prev, isConnected: true, retryCount: 0 }));
    } catch (error) {
      console.error('Error toggling motor:', error);
      setConnectionError('Failed to control motor');
      setConnectionStatus(prev => ({ ...prev, isConnected: false }));
    }
  };

  // const toggleAutoMode = () => setIsAutoMode(!isAutoMode);
  const toggleAutoMode = async () => {
    setIsAutoMode(prev => !prev); // Toggle locally first
    try {
      const response = await fetch('/api', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          command: "set_auto_mode",
          enabled: !isAutoMode, // Send the new state
        }),
      });

      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
      const data = await response.json();
      if (!data.success) throw new Error(data.message || 'Failed to set auto mode');
    } catch (error) {
      console.error('Error toggling auto mode:', error);
      setConnectionError('Failed to update auto mode');
    }
  };

  const retryConnection = () => {
    setConnectionStatus(prev => ({
      ...prev,
      retryCount: prev.retryCount + 1
    }));
    fetchSensorData();
  };

  const fetchSensorData = async () => {
    try {
      setConnectionError(null);
  
      // Update pH sensor
      await fetch('/api', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify({ command: "read_ph" })
      });
  
      await new Promise(resolve => setTimeout(resolve, 1000));
  
      const response = await fetch('/api', {
        headers: {
          'Accept': 'application/json'
        }
      });
  
      if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
  
      const data = await response.json();
      if (!data.success) throw new Error('Backend reported failure');
  
      const backendData = data.data;
      const newPhValue = backendData.current_ph || 7.0;
      const newEntry = {
        time: new Date(),
        value: newPhValue
      };
      
      setPhHistory(prev => [...prev.slice(-19), newEntry]); // Keep last 20 entries
      
      // setSensorData({
      const newSensorData = {
        ph: newPhValue,
        fishDetected: backendData.fish_detected || false,
        motorStatus: backendData.motor_initialized || false,
        current_ph_voltage: backendData.current_ph_voltage,
        feed_count: backendData.feed_count,
        auto_feed_count: backendData.auto_feed_count,
        last_feed_time: backendData.last_feed_time === "Never" ? undefined : backendData.last_feed_time,
	auto_last_feed_time: backendData.auto_last_feed_time === "Never" ? undefined : backendData.auto_last_feed_time,
        ph_sensor_initialized: backendData.ph_sensor_initialized
      };
  

      setSensorData(newSensorData);
      // Sync isAutoMode with fishDetected (overrides manual toggle)
      // setIsAutoMode(newSensorData.fishDetected);


      setConnectionStatus(prev => ({
        ...prev,
        isConnected: true,
        retryCount: 0,
        lastAttempt: new Date()
      }));
    } catch (error) {
      console.error('Error fetching sensor data:', error);
      setConnectionError(`Failed to connect: ${error instanceof Error ? error.message : 'Unknown error'}`);
      setConnectionStatus(prev => ({
        ...prev,
        isConnected: false,
        lastAttempt: new Date()
      }));
  
      if (connectionStatus.retryCount < MAX_RETRIES) {
        setTimeout(retryConnection, RETRY_DELAY);
      }
    }
  };

  useEffect(() => {
    fetchSensorData();
    const intervalId = setInterval(fetchSensorData, 5000);
    return () => clearInterval(intervalId);
  }, []);

  // Format time for chart display
  const formatTime = (date: Date) => {
    return date.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-blue-50 to-blue-100 p-4 md:p-8">
      <header className="mb-8">
        <h1 className="text-3xl font-bold text-blue-800">Aquarium Monitoring System</h1>
        <p className="text-blue-600">Real-time monitoring and control</p>
      </header>

      <div className="p-4 bg-white rounded-lg shadow mb-6">
        <div className="flex items-center justify-between">
          <h2 className="text-xl font-semibold">Connection Status</h2>
          <div className="flex items-center gap-2">
            {connectionStatus.isConnected ? (
              <span className="flex items-center text-green-600">
                <Wifi className="h-5 w-5 mr-1" />
                Connected
              </span>
            ) : (
              <span className="flex items-center text-red-600">
                <WifiOff className="h-5 w-5 mr-1" />
                Disconnected
              </span>
            )}
          </div>
        </div>
        {connectionError && (
          <div className="mt-2 flex items-center text-red-600">
            <AlertCircle className="h-4 w-4 mr-1" />
            {connectionError}
          </div>
        )}
      </div>
      
      <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
        {/* Camera Feed */}
        <div className="bg-white rounded-lg shadow-lg p-4">
          <h2 className="text-xl font-semibold mb-4">Camera Feed</h2>
          <div className="aspect-video bg-gray-200 rounded-lg overflow-hidden">
            <img 
              src="/video_feed" 
              alt="Live camera feed" 
              className="w-full h-full object-cover"
              onError={(e) => {
                (e.target as HTMLImageElement).src = 'placeholder-image.jpg';
              }}
            />
          </div>
        </div>
        
        {/* Controls */}
        <div className="bg-white rounded-lg shadow-lg p-4">
          <h2 className="text-xl font-semibold mb-4">System Controls</h2>
          
          <div className="space-y-4">
            {/* Auto Mode Toggle */}
            <div className="space-y-2">
              <button
                onClick={toggleAutoMode}
                className={`w-full py-3 px-4 rounded-lg font-medium flex items-center justify-center gap-2 ${
                  isAutoMode
                    ? 'bg-blue-100 text-blue-700 hover:bg-blue-200'
                    : 'bg-gray-100 text-gray-700 hover:bg-gray-200'
                }`}
              >
                <Settings className="h-5 w-5" />
                {isAutoMode ? 'Auto Mode (ON)' : 'Auto Mode (OFF)'}
              </button>
               {/* Timestamp for Auto Mode */}
                {sensorData.auto_last_feed_time && (
                  <div className="text-sm text-gray-500 mt-2">
                    Last feed time: {new Date(sensorData.auto_last_feed_time).toLocaleString()}
                  </div>
                )}
            </div>

            {/* Motor Control */}
            <div className="space-y-2">
              <button
                onClick={toggleMotor}
                className={`w-full py-3 px-4 rounded-lg font-medium flex items-center justify-center gap-2 ${
                  sensorData.motorStatus
                    ? 'bg-red-100 text-red-700 hover:bg-red-200'
                    : 'bg-green-100 text-green-700 hover:bg-green-200'
                }`}
              >
                <Power className="h-5 w-5" />
                {sensorData.motorStatus ? 'Dispense Feed Fish' : 'Run Motor'}
              </button>
                {/* Timestamp for Auto Mode */}
                {sensorData.last_feed_time && (
                    <div className="text-sm text-gray-500 mt-2">
                      Last feed time: {new Date(sensorData.last_feed_time).toLocaleString()}
                    </div>
                  )}
            </div>

            {/* pH Level Graph */}
            <div className="space-y-2">
              <div className="flex items-center gap-2 mb-2">
                <Droplets className="h-5 w-5 text-blue-500" />
                <span className="font-medium">Water pH Level</span>
                <span className="ml-auto font-bold text-blue-600">
                  {sensorData.ph.toFixed(1)}
                </span>
              </div>
              
              <div className="h-64">
                <ResponsiveContainer width="100%" height="100%">
                  <LineChart data={phHistory}>
                    <CartesianGrid strokeDasharray="3 3" />
                    <XAxis 
                      dataKey="time"
                      tickFormatter={(date) => formatTime(new Date(date))}
                    />
                    <YAxis domain={[0, 14]} />
                    <Tooltip 
                      labelFormatter={(value) => `Time: ${formatTime(new Date(value))}`}
                      formatter={(value) => [`pH: ${value}`, '']}
                    />
                    <Legend />
                    <Line 
                      type="monotone" 
                      dataKey="value" 
                      stroke="#3b82f6" 
                      strokeWidth={2}
                      dot={false}
                      activeDot={{ r: 6 }}
                      name="pH Level"
                    />
                  </LineChart>
                </ResponsiveContainer>
              </div>
              
              <div className="flex justify-between text-sm text-gray-500">
                {sensorData.ph_sensor_initialized !== undefined && (
                  <div>
                    Status: {sensorData.ph_sensor_initialized ? 
                      <span className="text-green-600">Sensor Active</span> : 
                      <span className="text-red-600">Sensor Offline</span>}
                  </div>
                )}
                {sensorData.current_ph_voltage !== undefined && (
                  <div>
                    Voltage: {sensorData.current_ph_voltage.toFixed(2)}V
                  </div>
                )}
              </div>
            </div>

            {/* Fish Detection and Feed Counters */}
            <div className="space-y-2">
              <div className="flex items-center justify-between">
                <div className="flex items-center gap-2">
                  <Fish className="h-5 w-5" />
                  <span className="font-medium">Fish Detection</span>
                </div>
                <span className={`px-4 py-2 rounded-full font-medium ${
                  sensorData.fishDetected
                    ? 'bg-green-100 text-green-700'
                    : 'bg-gray-100 text-gray-700'
                }`}>
                  {sensorData.fishDetected ? 'Detected' : 'Not Detected'}
                </span>
              </div>
              
              <div className="flex flex-col space-y-[2px]">
                {sensorData.auto_feed_count !== undefined && (
                  <div className="p-2 rounded-lg flex">
                    <div className="w-1/2 bg-gray-100 p-2">
                      <h3 className="font-semibold text-gray-700">Auto Feed</h3>
                    </div>
                    <div className="w-1/2 bg-gray-300 p-2 text-gray-700">
                      <div className="text-sm">Count: {sensorData.auto_feed_count}</div>
                    </div>
                  </div>
                )}
                {sensorData.feed_count !== undefined && (
                  <div className="p-2 rounded-lg flex">
                    <div className="w-1/2 bg-gray-100 p-2">
                      <h3 className="font-semibold text-gray-700">Manual Feed</h3>
                    </div>
                    <div className="w-1/2 bg-gray-300 p-2 text-gray-700">
                      <div className="text-sm">Count: {sensorData.feed_count}</div>
                    </div>
                  </div>
                )}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;