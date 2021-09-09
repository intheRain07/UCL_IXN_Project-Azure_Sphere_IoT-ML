#!/usr/bin/env python
# coding: utf-8

# ## Workspace

# In[14]:
from azureml.core import Workspace

ws = Workspace.from_config()

print(ws)


# ## Environment

# In[15]:
from azureml.core import Environment

sklearn_env = Environment.get(workspace=ws, name='AzureML-Tutorial')


# ## Train my poly regression model on instance.

# In[16]:
import joblib 
import pandas as pd

import sklearn
from sklearn.linear_model import LinearRegression
from sklearn.linear_model import Ridge

# Import data munging tools
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import PolynomialFeatures

# Import error metric
from sklearn.metrics import mean_absolute_error

data = pd.read_csv('./people_in_supermarket_v2.csv')
X = data.iloc[:, 0:-1]
y = data['numOfpeopleNextmin']

x_train, x_test, y_train, y_test = train_test_split(X, y, test_size = 0.2, random_state = 0)

# redige regression
poly_features = PolynomialFeatures(degree = 11) 
# Transform training and test datasets 
x_train_poly = poly_features.fit_transform(x_train)
x_test_poly = poly_features.fit_transform(x_test)

lambda_value = 1E-15
ridge_reg = Ridge(alpha = lambda_value, normalize = True)
# Fit the model
ridge_reg.fit(x_train_poly,y_train)

joblib.dump(ridge_reg, "0073_poly_regression_model.pkl") # store the trained model in pkl file 


# In[17]:
# Compute the predictions of the model for test set
y_reg_test_pred = ridge_reg.predict(x_test_poly)
# Compute the Mean Absoluate Error for test set, then append the value to the lists 
abses_test_error = mean_absolute_error(y_test, y_reg_test_pred)

print(abses_test_error) 


# ## Register model

# In[31]:
from azureml.core import Model
from azureml.core.resource_configuration import ResourceConfiguration

#-------------------my poly LR model----------------------
model = Model.register(model_path="0073_poly_regression_model.pkl",
                       model_name="0073_poly_regression_model",
                       description="0073 Poly ridge regression model",
                       workspace=ws)


# ## Deployment

# Create environment

# In[40]:
import sklearn

from azureml.core.environment import Environment

environment = Environment("LocalDeploy")
environment.python.conda_dependencies.add_pip_package("inference-schema[numpy-support]")
environment.python.conda_dependencies.add_pip_package("joblib")
environment.python.conda_dependencies.add_pip_package("scikit-learn=={}".format(sklearn.__version__))


# Configure

# In[41]:
# Define an inference configuration
from azureml.core import Environment
from azureml.core.model import InferenceConfig

# -------------------Inference config--------------------------
inference_config = InferenceConfig(
    environment = environment,
    entry_script="score.py"
)

# -------------------Local Deploy config--------------------------
from azureml.core.webservice import LocalWebservice

deployment_config = LocalWebservice.deploy_configuration()

# -------------------WEB Deploy config--------------------------
from azureml.core.webservice import AciWebservice

web_deployment_config = AciWebservice.deploy_configuration(
    cpu_cores = 0.5, memory_gb = 1, auth_enabled = False
)


# Local Deploy

# In[43]:
# deploy
local_service = Model.deploy(
    ws, 
    "mscproj-poly-regression-model", 
    [model], 
    inference_config,
    deployment_config,
    overwrite = True 
)

local_service.wait_for_deployment(show_output=True)


# In[44]:
import requests
import json

uri = local_service.scoring_uri
print(uri)
headers = {"Content-Type": "application/json"}
data = {
    "data": [[5, 14, 32, 60]] 
}
data_json = json.dumps(data)
response = requests.post(uri, data = data_json, headers = headers)
print(response.json()[0])


# In[45]:
# test by model traind by instance
x_poly = poly_features.fit_transform([[5, 14, 32, 60]]) 
print(ridge_reg.predict(x_poly)) 


# Web Deploy

# In[46]:
# WEB deploy
web_service = Model.deploy(
    ws,
    "mscproj-ml-webservice",
    [model],
    inference_config,
    web_deployment_config,
    overwrite = True,
)
web_service.wait_for_deployment(show_output=True)


# WEB update

# In[ ]:
# web_service.update(
#     models = [model], 
#     inference_config = inference_config,
#     deployment_config = web_deployment_config
#     )


# WEB service test

# In[47]:


print(f'URL: {web_service.scoring_uri}') 


# In[48]:
import requests
import json

uri = web_service.scoring_uri
print(uri)
headers = {"Content-Type": "application/json"}
data = {
   "data": [[4, 12, 17, 30]] 
}
data_json = json.dumps(data)
response = requests.post(uri, data = data_json, headers = headers)
print(response.json())


# In[ ]:
x_poly = poly_features.fit_transform([[4, 12, 17, 30]]) 
print(ridge_reg.predict(x_poly)) 

