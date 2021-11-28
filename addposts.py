from os import listdir
blogname = 'Blog'
lines = []
fr = open('public/index.html', 'r')
for l in fr.readlines():
	lines.append(l)

fw = open('public/index.html', 'w')
for l in lines:
	if 'id=\"post-item\"' not in l:
		fw.write(l)
		if 'id=\"posts\"' in l:
			for f in listdir('public/posts'):
				fw.write('\t\t\t\t<li id=\"post-item\" onclick=\"location.href = \'{}\';\">{}</li>\n'.format('/posts/{}'.format(f), f[:-5]))
