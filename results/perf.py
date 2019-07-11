
import sys, re
import pandas as pd
from matplotlib import pyplot as plt

print("gathering data and generating dataframe")
filenames = [filename for filename in sys.argv[1:]]
data = pd.concat(
        [pd.read_csv(
            filename,
            header=None,
            index_col=[0, 1],
            names=['Local Workgroup Size', 'Number of Spheres', 'Compute Dispatch', 'Draw Call'],
        ) for filename in filenames],
        keys=[re.search('performance-(.+?)-', filename).group(1).replace('_', ' ').title() for filename in filenames],
        names=['Method']
)

print("plotting compute dispatch duration for ...")
print("... different methods with a local workgroup size of 128")
print("... different methods in general")
print("... different local workgroup sizes")
for num_spheres in data.index.levels[2]:
    current_data = data.reorder_levels([2, 1, 0])
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[128]
            .loc[['Euler Naive', 'Euler Interleaved', 'Euler Base']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_base-lwgs128-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[128]
            .loc[['Euler Nosoften', 'Euler Base']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_soften-lwgs128-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[128]
            .loc[['Euler Base', 'Euler Shared', 'Euler Shared Prefetch']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_shared-lwgs128-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[128]
            .loc[['Euler Shared Prefetch', 'Heun Shared Prefetch', 'Verlet Shared Prefetch']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_acc-lwgs128-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)

    current_data = data.reorder_levels([2, 0, 1])
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[['Euler Naive', 'Euler Interleaved', 'Euler Base']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_base-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[['Euler Nosoften', 'Euler Base']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_soften-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[['Euler Base', 'Euler Shared', 'Euler Shared Prefetch']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_shared-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)
    fig = (current_data['Compute Dispatch']
            .loc[num_spheres]
            .loc[['Euler Shared Prefetch', 'Heun Shared Prefetch', 'Verlet Shared Prefetch']]
            .sort_values(ascending=False)
            .plot.barh()
            .get_figure()
    )
    fig.savefig("perf-methods_acc-nos{}.png".format(num_spheres), bbox_inches='tight')
    plt.close(fig)

    for method in current_data.index.levels[1]:
        fig = (current_data['Compute Dispatch']
                .loc[num_spheres]
                .loc[method]
                .plot.bar()
                .get_figure()
        )
        fig.savefig("perf-lwgs-{}-nos{}.png".format(method.replace(' ', '_').lower(), num_spheres), bbox_inches='tight')
        plt.close(fig)

print("plotting comparison between compute dispatch duration and draw command duration")
current_data = data
for method in current_data.index.levels[0]:
    for local_workgroup_size in current_data.index.levels[1]:
        fig = (current_data
                .loc[method]
                .loc[local_workgroup_size]
                .loc[2:65536]
                .plot()
                .get_figure()
        )
        fig.savefig("perf-breakeven_low-{}-lwgs{}.png".format(method.replace(' ', '_').lower(), local_workgroup_size), bbox_inches='tight')
        plt.close(fig)
        fig = (current_data
                .loc[method]
                .loc[local_workgroup_size]
                .plot()
                .get_figure()
        )
        fig.savefig("perf-breakeven-{}-lwgs{}.png".format(method.replace(' ', '_').lower(), local_workgroup_size), bbox_inches='tight')
        plt.close(fig)


