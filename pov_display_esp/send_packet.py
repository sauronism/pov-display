import requests

esp_address = '192.168.4.1'
# Step 2: Prepare your JSON data
data = {
    'display_on': True,
    'eye_azimuth': 0,
    'display_custom_text': False,
    'custom_text_data': "hello world"
}

# Step 3: Make the POST request
response = requests.post(f"http://{esp_address}/json_client", json=data)

# Print the response text
print(response.text)
