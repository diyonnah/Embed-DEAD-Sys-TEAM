import pandas as pd
from sklearn.linear_model import LogisticRegression
from sklearn.model_selection import train_test_split

data = pd.read_csv("environment_dataset_50k.csv")

X = data[["Temperature","Humidity"]]
y = data["Label"]

X_train,X_test,y_train,y_test = train_test_split(X,y,test_size=0.2)

model = LogisticRegression()

model.fit(X_train,y_train)

accuracy = model.score(X_test,y_test)

print("Model Accuracy:",accuracy)

print("Model Weights:",model.coef_)

print("Model Bias:",model.intercept_)

