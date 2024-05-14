import requests

esp_address = '192.168.4.1'
# Step 2: Prepare your JSON data
data = {
    "x": "10",
    "y": "20",
    "z": "30"
}

# Step 3: Make the POST request
response = requests.post(f"http://{esp_address}/json_client", json=data)

# Print the response text
print(response.text)
