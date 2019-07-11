
import sys, re
import pandas as pd
from matplotlib import pyplot as plt

filenames = [filename for filename in sys.argv[1:]]
data = pd.concat(
        [pd.read_csv(
            filename,
            header=None,
            names=['Angular Momentum X', 'Angular Momentum Y', 'Angular Momentum Z', 'Total Energy', 'Total Force Start', 'Total Force End']
        ) for filename in filenames],
        keys=[re.search('accuracy-(.+?)_', filename).group(1).title() for filename in filenames],
        names=['Method']
)

data = data.droplevel(1)

fig = data['Angular Momentum Y'].plot.bar(logy=True, ylim=1e0).get_figure()
fig.savefig('acc-avg-angmom.png', bbox_inches='tight')
plt.close(fig)

fig = data['Total Energy'].plot.bar(logy=True, ylim=1e0).get_figure()
fig.savefig('acc-avg-energy.png', bbox_inches='tight')
plt.close(fig)

fig = data[['Total Force Start', 'Total Force End']].plot.bar(logy=True, ylim=1e0).get_figure()
fig.savefig('acc-avg-force.png', bbox_inches='tight')
plt.close(fig)


