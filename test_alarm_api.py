#!/usr/bin/env python3
"""
Test script for alarm configuration API endpoints
"""

import requests
import json
import sys

# Change this to your ESP32 IP address
ESP32_IP = "192.168.1.100"  # Update this!
BASE_URL = f"http://{ESP32_IP}"

def test_get_alarm_config():
    """Test GET /api/alarm-config endpoint"""
    print("\n=== Testing GET /api/alarm-config ===")
    try:
        response = requests.get(f"{BASE_URL}/api/alarm-config", timeout=5)
        print(f"Status Code: {response.status_code}")
        
        if response.status_code == 200:
            data = response.json()
            print(f"Response: {json.dumps(data, indent=2)}")
            
            # Check structure
            if 'points' in data:
                print(f"\nFound {len(data['points'])} measurement points")
                
                # Check first point structure
                if data['points']:
                    point = data['points'][0]
                    required_fields = [
                        'address', 'name', 'currentTemp', 'sensorBound',
                        'lowThreshold', 'highThreshold', 'hysteresis',
                        'lowEnabled', 'highEnabled', 'errorEnabled',
                        'lowPriority', 'highPriority', 'errorPriority'
                    ]
                    
                    print(f"\nFirst point structure:")
                    for field in required_fields:
                        if field in point:
                            print(f"  ✓ {field}: {point[field]}")
                        else:
                            print(f"  ✗ {field}: MISSING")
            else:
                print("ERROR: No 'points' field in response")
        else:
            print(f"ERROR: HTTP {response.status_code}")
            print(response.text)
            
    except requests.exceptions.RequestException as e:
        print(f"ERROR: Request failed - {e}")
        print(f"Make sure to update ESP32_IP in the script!")

def test_post_alarm_config():
    """Test POST /api/alarm-config endpoint"""
    print("\n=== Testing POST /api/alarm-config ===")
    
    # Test data
    test_changes = {
        "changes": [
            {
                "address": 0,
                "name": "Test Point 0",
                "lowThreshold": -5.0,
                "highThreshold": 85.0,
                "lowPriority": 2,
                "highPriority": 3,
                "errorPriority": 3,
                "lowEnabled": True,
                "highEnabled": True,
                "errorEnabled": False,
                "hysteresis": 3.0
            }
        ]
    }
    
    try:
        response = requests.post(
            f"{BASE_URL}/api/alarm-config",
            json=test_changes,
            headers={'Content-Type': 'application/json'},
            timeout=5
        )
        
        print(f"Status Code: {response.status_code}")
        
        if response.status_code == 200:
            data = response.json()
            print(f"Response: {json.dumps(data, indent=2)}")
            
            # Verify the change by getting config again
            print("\nVerifying changes...")
            verify_response = requests.get(f"{BASE_URL}/api/alarm-config", timeout=5)
            if verify_response.status_code == 200:
                verify_data = verify_response.json()
                if verify_data['points']:
                    point = next((p for p in verify_data['points'] if p['address'] == 0), None)
                    if point:
                        print(f"Point 0 after update:")
                        print(f"  Name: {point.get('name')}")
                        print(f"  Low threshold: {point.get('lowThreshold')}")
                        print(f"  High threshold: {point.get('highThreshold')}")
                        print(f"  Low enabled: {point.get('lowEnabled')}")
                        print(f"  High enabled: {point.get('highEnabled')}")
                        print(f"  Hysteresis: {point.get('hysteresis')}")
        else:
            print(f"ERROR: HTTP {response.status_code}")
            print(response.text)
            
    except requests.exceptions.RequestException as e:
        print(f"ERROR: Request failed - {e}")

def main():
    print(f"Testing alarm configuration API on {ESP32_IP}")
    print("=" * 50)
    
    if len(sys.argv) > 1:
        ESP32_IP = sys.argv[1]
        print(f"Using IP address from command line: {ESP32_IP}")
    
    test_get_alarm_config()
    test_post_alarm_config()
    
    print("\n" + "=" * 50)
    print("Test complete!")

if __name__ == "__main__":
    main()