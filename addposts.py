from os import listdir
blogname = 'Blog'
lines = []
fr = open('public/index.html', 'r')
for l in fr.readlines():
	lines.append(l)

fw = open('public/index.html', 'w')
for l in lines:
	if 'list-group-item' not in l:
		fw.write(l)
		if '<ul' in l:
			for f in listdir('public/posts'):
				fw.write('\t\t\t\t<a href=\"{}\" class=\"list-group-item list-group-item-action\">{}</a>\n'.format('/posts/{}'.format(f), f[:-5]))
