from os import listdir
blogname = 'Blog'
lines = []
fr = open('docs/index.html', 'r')
for l in fr.readlines():
	lines.append(l)

fw = open('docs/index.html', 'w')
for l in lines:
	if 'id=\"post-item\"' not in l:
		fw.write(l)
		if 'id=\"posts\"' in l:
			for f in listdir('docs/posts'):
				fw.write('\t\t\t\t<li id=\"post-item\"><a href=\"{}\" target=\"blogpost\">{}</a></li>\n'.format('/blog/posts/{}'.format(f), ' '.join(f.split('-'))[:-5]))
