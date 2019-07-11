
import sys, re
import pandas as pd
from matplotlib import pyplot as plt

filenames = [filename for filename in sys.argv[1:]]
data = pd.concat(
        [pd.read_csv(
            filename,
            header=None,
            index_col=[0],
            names=['Number of Spheres', 'Angular Momentum X', 'Angular Momentum Y', 'Angular Momentum Z', 'Total Energy', 'Total Force Start', 'Total Force End']
        ) for filename in filenames],
        keys=[re.search('accuracy-(.+?)_', filename).group(1).title() for filename in filenames],
        names=['Method']
)

data['Angular Momentum Y'].loc['Euler'].plot(style='blue')
data['Angular Momentum Y'].loc['Heun'].plot(style='red')
axes = data['Angular Momentum Y'].loc['Verlet'].plot(style='orange')
fig = axes.legend(['Euler', 'Heun', 'Verlet']).get_figure()
fig.savefig('acc-nos-angmom.png', bbox_inches='tight')
plt.close(fig)

data['Total Energy'].loc['Euler'].plot(style='blue')
data['Total Energy'].loc['Heun'].plot(style='red')
axes = data['Total Energy'].loc['Verlet'].plot(style='orange')
fig = axes.legend(['Euler', 'Heun', 'Verlet']).get_figure()
fig.savefig('acc-nos-energy.png', bbox_inches='tight')
plt.close(fig)

data['Angular Momentum Y'].loc['Euler'].plot(logy=True, style='blue')
data['Angular Momentum Y'].loc['Heun'].plot(logy=True, style='red')
axes = data['Angular Momentum Y'].loc['Verlet'].plot(logy=True, style='orange')
fig = axes.legend(['Euler', 'Heun', 'Verlet']).get_figure()
fig.savefig('acc-nos-angmom-log.png', bbox_inches='tight')
plt.close(fig)

data['Total Energy'].loc['Euler'].plot(logy=True, style='blue')
data['Total Energy'].loc['Heun'].plot(logy=True, style='red')
axes = data['Total Energy'].loc['Verlet'].plot(logy=True, style='orange')
fig = axes.legend(['Euler', 'Heun', 'Verlet']).get_figure()
fig.savefig('acc-nos-energy-log.png', bbox_inches='tight')
plt.close(fig)

axes = data[['Total Force Start', 'Total Force End']].loc['Euler'].plot.bar(logy=True)
fig = axes.legend().get_figure()
fig.savefig('acc-nos-force-euler.png', bbox_inches='tight')
plt.close(fig)

axes = data[['Total Force Start', 'Total Force End']].loc['Heun'].plot.bar(logy=True)
fig = axes.legend().get_figure()
fig.savefig('acc-nos-force-heun.png', bbox_inches='tight')
plt.close(fig)

axes = data[['Total Force Start', 'Total Force End']].loc['Verlet'].plot.bar(logy=True)
fig = axes.legend().get_figure()
fig.savefig('acc-nos-force-verlet.png', bbox_inches='tight')
plt.close(fig)

