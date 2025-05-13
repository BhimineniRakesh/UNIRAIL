import pandas as pd

csv_file = "Trames3.csv"   # Replace name with your CSV files
df = pd.read_csv(csv_file)

def get_can48_indices(csv_file):
    df = pd.read_csv(csv_file)
    can48_indices = df[df['canid'] == 48].index.tolist()
    return can48_indices

balises = get_can48_indices(csv_file)
power_surge_count = {}

for index, row in df.iterrows():
    can_id = row['canid']
    if can_id == 32:
        if row['data1'] == 2:
            cnt += 1
            print("Power Surge Detected:", cnt)
            indexloc = index
            while df.at[indexloc, 'canid'] != 291:
                indexloc -= 1

            x_msb = df.at[indexloc, 'data0']
            x_second = df.at[indexloc, 'data1']
            x_third = df.at[indexloc, 'data2']
            x_lsb = df.at[indexloc, 'data3']

            y_msb = df.at[indexloc, 'data4']
            y_second = df.at[indexloc, 'data5']
            y_third = df.at[indexloc, 'data6']
            y_lsb = df.at[indexloc, 'data7']

            x = (x_msb << 24) | (x_second << 16) | (x_third << 8) | x_lsb
            y = (y_msb << 24) | (y_second << 16) | (y_third << 8) | y_lsb

            print(f"Power surge detected at time: {df.at[index, 'Timestamp']}")
            print(f"Location (x, y): ({x}, {y})")

            tmp = [abs(num - index) for num in balises]
            tmp2 = [num - index for num in balises]
            indexbal = tmp2[tmp.index(min(tmp))] + index
            balisenum = df.at[indexbal, 'data5']
            print(f"Balise number: {balisenum}")
            print("----------------------------------\n\n")

            if balisenum not in power_surge_count:
                power_surge_count[balisenum] = 1
            else:
                power_surge_count[balisenum] += 1

# Find the balise with the most power surges
max_power_surge_balise = max(power_surge_count, key=power_surge_count.get)
max_power_surge_count = power_surge_count[max_power_surge_balise]

print(f"Balise with the most power surges: {max_power_surge_balise}")
print(f"Number of power surges: {max_power_surge_count}")