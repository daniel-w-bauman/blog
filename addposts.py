from os import listdir
blogname = 'Blog'

fw = open('index.html', 'w')
fw.write('<!DOCTYPE html>\n')
fw.write('<html>\n')
fw.write('\t<head>\n')
fw.write('\t\t<title>{}</title>\n'.format(blogname))
fw.write('\t</head>\n')
fw.write('\t<body>\n')
fw.write('\t\t<h1>{}</h1>'.format(blogname))
fw.write('\t\t<ul>\n')
for f in listdir('posts'):
	fw.write('\t\t<li><a href=\"{}\">{}</a></li>\n'.format('/posts/{}'.format(f), f[:-5]))
fw.write('\t\t</ul>\n')
fw.write('\t</body>\n')
fw.write('</html>\n')
