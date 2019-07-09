
import sys, re
import pandas as pd
from matplotlib import pyplot as plt

filenames = [filename for filename in sys.argv[1:]]
data = pd.concat(
        [pd.read_csv(
            filename,
            header=None,
            index_col=[0],
            names=['number of spheres', 'total energy', 'angular momentum', 'total force']
        ) for filename in filenames],
        keys=[re.search('accuracy-(.+?)_', filename).group(1).title() for filename in filenames],
        names=['Method']
)


current_data = data.reorder_levels([1, 0])
print(current_data)
for num_spheres in current_data.index.levels[0]:
    fig = (current_data
            .loc[num_spheres]
            .plot.bar(logy=True)
            .get_figure()
    )
    fig.savefig('methods_{}nos.png'.format(num_spheres), bbox_inches='tight')


