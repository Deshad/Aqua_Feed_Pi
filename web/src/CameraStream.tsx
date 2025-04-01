import React, { useState, useEffect } from 'react';

const CameraStream = () => {
  const [imgSrc, setImgSrc] = useState('/video_feed');
  const [error, setError] = useState(false);

  useEffect(() => {
    const interval = setInterval(() => {
      setImgSrc(`/video_feed?t=${Date.now()}`);
    }, 200); // Refresh every 200ms (5fps)

    return () => clearInterval(interval);
  }, []);

  return (
    <div className="bg-white rounded-lg shadow-lg p-4">
      <h2 className="text-xl font-semibold mb-4">Camera Feed</h2>
      <div className="aspect-video bg-gray-200 rounded-lg overflow-hidden">
        {error ? (
          <div className="w-full h-full flex items-center justify-center">
            <p className="text-red-500">Camera feed unavailable</p>
          </div>
        ) : (
          <img 
            src={imgSrc}
            alt="Live camera feed"
            className="w-full h-full object-cover"
            onError={() => setError(true)}
            onLoad={() => setError(false)}
          />
        )}
      </div>
    </div>
  );
};

export default CameraStream;