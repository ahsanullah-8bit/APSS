import os
import requests
from pprint import pprint


regions = ["mx", "us-ca"] # Change to your country
with open('../test/assets/vehicles.jpg', 'rb') as fp:
    api_key = os.environ["PLATE_RECOGNIZER_API"]
    response = requests.post(
        'https://api.platerecognizer.com/v1/plate-reader/',
        data=dict(regions=regions),  #Optional
        files=dict(upload=fp),
        headers={'Authorization': f'Token {api_key}'})

pprint(response.json())

# vehicles2.jpg
# {'camera_id': None,                                                                                                                                                  
#  'filename': '1055_77QvN_vehicles2.jpg',
#  'image_height': 1280,
#  'image_width': 960,
#  'processing_time': 89.726,
#  'results': [{'box': {'xmax': 700, 'xmin': 607, 'ymax': 591, 'ymin': 544},
#               'candidates': [{'plate': 'mw776', 'score': 0.999}],
#               'dscore': 0.867,
#               'plate': 'mw776',
#               'region': {'code': 'unknown', 'score': 0.011},
#               'score': 0.999,
#               'vehicle': {'box': {'xmax': 837,
#                                   'xmin': 375,
#                                   'ymax': 670,
#                                   'ymin': 274},
#                           'score': 0.94,
#                           'type': 'Sedan'}}],
#  'timestamp': '2025-07-14T10:55:50.022199Z',
#  'version': 1}

# vehicles.jpg
# {'camera_id': None,                                                                                                                                                  
#  'filename': '1100_AToyC_vehicles.jpg',
#  'image_height': 1280,
#  'image_width': 960,
#  'processing_time': 246.305,
#  'results': [{'box': {'xmax': 612, 'xmin': 523, 'ymax': 698, 'ymin': 642},
#               'candidates': [{'plate': 'ajz254', 'score': 0.998}],
#               'dscore': 0.831,
#               'plate': 'ajz254',
#               'region': {'code': 'ph', 'score': 0.165},
#               'score': 0.998,
#               'vehicle': {'box': {'xmax': 789,
#                                   'xmin': 283,
#                                   'ymax': 935,
#                                   'ymin': 453},
#                           'score': 0.631,
#                           'type': 'Sedan'}},
#              {'box': {'xmax': 928, 'xmin': 900, 'ymax': 475, 'ymin': 455},
#               'candidates': [{'plate': 'a1172', 'score': 0.98},
#                              {'plate': 'ai172', 'score': 0.797},
#                              {'plate': 'a1i72', 'score': 0.79},
#                              {'plate': 'aii72', 'score': 0.608}],
#               'dscore': 0.695,
#               'plate': 'a1172',
#               'region': {'code': 'unknown', 'score': 0.011},
#               'score': 0.98,
#               'vehicle': {'box': {'xmax': 958,
#                                   'xmin': 681,
#                                   'ymax': 591,
#                                   'ymin': 253},
#                           'score': 0.773,
#                           'type': 'Bus'}}],
#  'timestamp': '2025-07-14T11:00:12.524945Z',
#  'version': 1}