import os
import json
import joblib
import numpy as np
from sklearn.preprocessing import PolynomialFeatures


def init():
    print("This is init.")
    global model
    model_path = os.path.join(os.getenv('AZUREML_MODEL_DIR'), '0073_poly_regression_model.pkl')
    model = joblib.load(model_path)


def run(data):

    data_obj = json.loads(data)
    
    data_list = data_obj.get('data')
    
    poly_features = PolynomialFeatures(degree = 11)
    input_poly = poly_features.fit_transform(data_list) 

    try:
        result = model.predict(input_poly) 
        return result.tolist()
    except Exception as e:
        error = str(e)
        return error


