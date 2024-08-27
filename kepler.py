# %%
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from sklearn.cluster import KMeans

# from sklearn.metrics import silhouette_score

# 读取CSV文件
file_path = "OneWebData.csv"
df = pd.read_csv(file_path)

# Select the specified columns
selected_columns = [
    "Inclination (radius )",
    "Right Ascension of the Ascending Node (radius )",
    "Argument of Periapsis  (radius )",
]
print(df.head())
df_selected = df[selected_columns]

# Display the first few rows of the selected columns
print(df_selected.head())


# %%
# 提取需要的列
inclination = df["Inclination (radius )"].values
raan = df["Right Ascension of the Ascending Node (radius )"].values
arg_perigee = df["Argument of Periapsis  (radius )"].values

# 将数据组合成一个二维数组
data = np.column_stack((raan,))

# 选择最佳的聚类数量
# range_n_clusters = range(2, 13)
# best_n_clusters = 2
# best_silhouette_score = -1

# for n_clusters in range_n_clusters:
#     kmeans = KMeans(n_clusters=n_clusters)
#     labels = kmeans.fit_predict(data)
#     silhouette_avg = silhouette_score(data, labels)
#     print(
#         f"For n_clusters = {n_clusters}, the average silhouette_score is : {silhouette_avg}"
#     )
#     if silhouette_avg > best_silhouette_score:
#         best_n_clusters = n_clusters
#         best_silhouette_score = silhouette_avg

# print(
#     f"Best number of clusters: {best_n_clusters} with silhouette score: {best_silhouette_score}"
# )
# %%

# 设置聚类的数量（可以调整）
num_clusters = 12

# 进行KMeans聚类
kmeans = KMeans(n_clusters=num_clusters)
kmeans.fit(data)

# 获取聚类结果
labels = kmeans.labels_
centroids = kmeans.cluster_centers_

# %%
# 创建一个三维图形
fig = plt.figure()
ax = fig.add_subplot(111, projection="3d")

three_d_data = np.column_stack((inclination, raan, arg_perigee))
# 使用不同颜色绘制不同聚类的点
for i in range(num_clusters):
    cluster_points = three_d_data[labels == i]
    ax.scatter(
        cluster_points[:, 0],
        cluster_points[:, 1],
        cluster_points[:, 2],
        label=f"Cluster {i+1}",
    )
#
# 绘制聚类中心
# ax.scatter(
#     centroids[:, 0],
#     centroids[:, 1],
#     centroids[:, 2],
#     c="black",
#     marker="x",
#     s=100,
#     label="Centroids",
# )

# 添加坐标轴标签
ax.set_xlabel("Inclination (degrees)")
ax.set_ylabel("RAAN (degrees)")
ax.set_zlabel("Argument of Perigee (degrees)")

# 显示图例
ax.legend()
#
# 显示图形
plt.show()

# %%
# Append labels to the original dataframe under column orbit_index
df["orbit_index"] = labels

# Display the first few rows of the updated dataframe to verify the new column
print(df[["SATNAME, ID and Timestamp ", "orbit_index"]].head())

# Optional: Save the updated dataframe to a new CSV file
df.to_csv("oneweb_data_with_orbits.csv", index=False)

# %%
