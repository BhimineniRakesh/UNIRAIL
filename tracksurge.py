import pandas as pd

def detect_power_surge_locations(csv_files):
    power_surge_locations = []

    for csv_file in csv_files:
        df = pd.read_csv(csv_file)
        power_surge_indices = df[(df['canid'] == 32) & (df['data1'] == 2)].index.tolist()

        for index in power_surge_indices:
            x, y, timestamp = extract_location(df, index)
            power_surge_locations.append((x, y, timestamp))

    return power_surge_locations

def extract_location(df, index):
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

    timestamp = df.at[index, 'Timestamp']
    return x, y, timestamp

def detect_track_surges(csv_files):
    power_surge_locations = detect_power_surge_locations(csv_files)
    track_surge_locations = []

    for location in power_surge_locations:
        x, y, _ = location
        count = sum((loc[0] == x and loc[1] == y) for loc in power_surge_locations)
        if count == len(csv_files):
            track_surge_locations.append(location)

    return track_surge_locations

csv_files = ["Trames2_1.csv", "Trames3_2.csv", "Trames3_5.csv"]  # Replace with your three CSV files
track_surges = detect_track_surges(csv_files)

if len(track_surges) > 0:
    print("Track surges detected at the following locations:")
    for x, y, timestamp in track_surges:
        print(f"Location (x, y): ({x}, {y})")
        print(f"Timestamp: {timestamp}")
        print("----------------------------------")
else:
    print("No track surges detected.")


print(len(track_surges))