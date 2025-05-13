



#1st cell
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score, f1_score
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, LSTM
from tensorflow.keras.layers import Dropout
from sklearn.model_selection import cross_val_score, KFold

csv_files = ["Trames3_2.csv"] # Replace with your CSV files

combined_df = pd.DataFrame()

for file in csv_files:
    df = pd.read_csv(file)
    combined_df = pd.concat([combined_df, df], ignore_index=True)


df_can32 = combined_df[combined_df['canid'] == 32]

X = df_can32[['data0', 'data2', 'data3', 'data4', 'data5', 'data6', 'data7']].values
y = df_can32['data1'].values

y_binary = np.where(y == 2, 1, 0)

X_train, X_test, y_train, y_test = train_test_split(X, y_binary, test_size=0.2)

scaler = StandardScaler()
X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

X_train_reshaped = X_train_scaled.reshape(X_train_scaled.shape[0], X_train_scaled.shape[1], 1)
X_test_reshaped = X_test_scaled.reshape(X_test_scaled.shape[0], X_test_scaled.shape[1], 1)

model = Sequential()
model.add(LSTM(128, input_shape=(X_train_reshaped.shape[1], X_train_reshaped.shape[2])))
model.add(Dense(1, activation='sigmoid'))


model.compile(loss='binary_crossentropy', optimizer='adam', metrics=['accuracy'])

model.fit(X_train_reshaped, y_train, epochs=50, batch_size=32, validation_data=(X_test_reshaped, y_test))

y_pred = model.predict(X_test_reshaped)
y_pred_binary = np.round(y_pred).flatten()

accuracy = accuracy_score(y_test, y_pred_binary)
print("Test Accuracy:", accuracy)
f1 = f1_score(y_test, y_pred_binary)
print("F1 Score:", f1)

def preprocess_data(df):
    X = df[['data0', 'data2', 'data3', 'data4', 'data5', 'data6', 'data7']].values
    scaler = StandardScaler()
    X_scaled = scaler.fit_transform(X)
    X_reshaped = X_scaled.reshape(X_scaled.shape[0], X_scaled.shape[1], 1)
    return X_reshaped

combined_dfPred = pd.DataFrame()

for file in csv_files:
  df = pd.read_csv(file)
  combined_dfPred = pd.concat([combined_df, df],ignore_index=True)

df_can32Pred = combined_dfPred[combined_dfPred['canid'] == 32]

df32_prep = preprocess_data(df_can32Pred)

pred = model.predict(df32_prep)

pred = np.round(pred,0)


#2nd cell

def get_can48_indices(combined_dfPred):
    can48_indices = combined_dfPred[combined_dfPred['canid'] == 48].index.tolist()
    return can48_indices

balises = get_can48_indices(combined_dfPred)

cnt = 0
cntpred = 0
actual = 0
predr = 0

# Open the output file in write mode
with open("output.txt", "w") as f:

    for index, row in combined_dfPred.iterrows():
        can_id = row['canid']
        if can_id == 32:
            cntpred += 1
            if pred[cntpred-1] == 1:
                cnt += 1
                print("Power Surge Detected: ", cnt)
                indexloc = index

                while combined_dfPred.at[indexloc, 'canid'] != 291:
                    indexloc += 1

                x_msb = combined_dfPred.at[indexloc, 'data0']
                x_second = combined_dfPred.at[indexloc, 'data1']
                x_third = combined_dfPred.at[indexloc, 'data2']
                x_lsb = combined_dfPred.at[indexloc, 'data3']

                y_msb = combined_dfPred.at[indexloc, 'data4']
                y_second = combined_dfPred.at[indexloc, 'data5']
                y_third = combined_dfPred.at[indexloc, 'data6']
                y_lsb = combined_dfPred.at[indexloc, 'data7']

                x = (x_msb << 24) | (x_second << 16) | (x_third << 8) | x_lsb
                y = (y_msb << 24) | (y_second << 16) | (y_third << 8) | y_lsb

                print(f"Power surge detected at time: {combined_dfPred.at[index, 'Timestamp']}")
                print(f"Location (x, y): ({x}, {y})")

                tmp = [abs(num - index) for num in balises]
                tmp2 = [num - index for num in balises]
                indexbal = tmp2[tmp.index(min(tmp))] + index
                balisenum = combined_dfPred.at[indexbal, 'data5']
                print(f"Balise number: {balisenum}")
                print("----------------------------------\n\n")

                # Write the print statements to the output file
                f.write(f"Power Surge Detected: {cnt}\n")
                f.write(f"Power surge detected at time: {combined_dfPred.at[index, 'Timestamp']}\n")
                f.write(f"Location (x, y): ({x}, {y})\n")
                f.write(f"Balise number: {balisenum}\n")
                f.write("----------------------------------\n\n")
				
#3rd cell
cnt = 0
cntpred = 0
actual = 0
predr = 0
for index, row in combined_dfPred.iterrows():
    can_id = row['canid']
    if can_id == 32:
        cntpred += 1
        if row['data1'] == 2:
          actual+=1;

        if pred[cntpred-1] == 1:
          predr += 1;

        if row['data1'] == 2 and pred[cntpred-1] == 1:
            cnt += 1
			print("Actual Power Surge Points : ",actual)
print("Predicted Power Serge Points: ",predr)
print("Both gon right at same loc : ",cnt);
