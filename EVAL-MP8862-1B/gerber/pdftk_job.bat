@FOR /F %%F IN ('dir /b *Edge_Cuts.pdf') DO @(
	@SET edgecuts=%%F
)   
@ECHO Adding to pdfs: %edgecuts%
@IF DEFINED edgecuts @(
	@FOR /F %%F IN ('dir /b *.pdf') DO @(
		@IF NOT "%%F"==%edgecuts% @(
			@ECHO : %%F
			pdftk "%%F" background %edgecuts% output "%%F_tmp.pdf" 
		)
	)  
)

pdftk *_tmp.pdf %edgecuts% cat output "../layout.pdf"
del *_tmp.pdf
