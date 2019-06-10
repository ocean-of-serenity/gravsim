
import sys, re
import pandas as pd
from matplotlib import pyplot as plt

filenames = [filename for filename in sys.argv[1:]]
data = pd.concat(
        [pd.read_csv(
            filename,
            header=None,
            index_col=[0],
            names=['number of spheres', 'total energy difference', 'angular momentum difference', 'total force difference']
        ) for filename in filenames],
        keys=[re.search('accuracy-(.+?)-', filename).group(1) for filename in filenames],
        names=['method']
)
print(data)

#current_data = data.reorder_levels([2, 0, 1])
#for num_spheres in current_data.index.levels[0]:
#    fig = (current_data
#            .loc[num_spheres]['gravity compute']
#            .sort_values(ascending=False)
#            .plot.barh()
#            .get_figure()
#    )
#    fig.savefig("gc_nos{}.png".format(num_spheres), bbox_inches='tight')
#    plt.close(fig)
#
#    for method in current_data.index.levels[1]:
#        fig = (current_data
#                .loc[num_spheres]
#                .loc[method]
#                .plot.bar()
#                .get_figure()
#        )
#        fig.savefig("nos-method_{}-{}.png".format(num_spheres, method), bbox_inches='tight')
#        plt.close(fig)
#
#current_data = data
#for method in current_data.index.levels[0]:
#    for local_workgroup_size in current_data.index.levels[1]:
#        fig = (current_data
#                .loc[method]
#                .loc[local_workgroup_size]
#                .plot()
#                .get_figure()
#        )
#        fig.savefig("method-lwgs_{}-{}.png".format(method, local_workgroup_size), bbox_inches='tight')
#        plt.close(fig)


